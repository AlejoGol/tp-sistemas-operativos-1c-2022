#include "cpu.h"
#include "serializado.h"
#include "utils-cliente.h"
#include "utils-servidor.h"
#include <libstatic.h> // <-- STATIC LIB
int CONEXION_CPU_INTERRUPT;
// void* escuchar_dispatch(void* arguments) {
void* escuchar_dispatch() {
  // struct arg_struct* args = (struct arg_struct*)arguments;
  estado_conexion_kernel = true;

  // memcpy(ip, args->arg1, strlen(args->arg1));
  // memcpy(puerto, args->arg2, strlen(args->arg2));
  char* ip = config_get_string_value(config, "IP_ESCUCHA");
  char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
  socket_cpu_dispatch = iniciar_servidor(ip, puerto);

  while (estado_conexion_kernel) {
    int cliente_fd = esperar_cliente(socket_cpu_dispatch);
    /*
        if (cliente_fd != -1) {
          t_paquete* paquete = paquete_create();
          t_buffer* mensaje = crear_mensaje("Conexión aceptada por CPU");

          paquete_cambiar_mensaje(paquete, mensaje), enviar_mensaje(cliente_fd, paquete);
          // paquete_add_mensaje(paquete, mensaje);
        }
    */
    pthread_t th;
    pthread_create(&th, NULL, manejar_nueva_conexion, &cliente_fd), pthread_detach(th);
  }
  free(ip);
  free(puerto);
  pthread_exit(NULL);
}

void* manejar_nueva_conexion(void* args) {
  int socket_cliente = *(int*)args;

  estado_conexion_con_cliente = true;

  while (estado_conexion_con_cliente) {
    int cod_op = recibir_operacion(socket_cliente);

    // MENSAJE=0, PAQUETE=1
    switch (cod_op) {
      case OPERACION_PCB: {
        t_paquete* paquete_con_pcb = malloc(sizeof(t_paquete) + 1);
        paquete_con_pcb = recibir_paquete(socket_cliente);

        t_pcb* pcb_deserializado = paquete_obtener_pcb(paquete_con_pcb);
        pcb_deserializado->socket = socket_cliente;
        ciclo_instruccion(pcb_deserializado, socket_cliente);
        imprimir_pcb(pcb_deserializado);
        paquete_destroy(paquete_con_pcb);
        free(pcb_deserializado);
        // descomentar para validar el memcheck
        // terminar_servidor(socket_cpu_dispatch, logger, config);
        // return 0;
      } break;
      case OPERACION_EXIT: {
        xlog(COLOR_CONEXION, "Se recibió solicitud para finalizar ejecución");

        log_destroy(logger), close(socket_cpu_dispatch);
        // TODO: no estaría funcionando del todo, queda bloqueado en esperar_cliente()
        estado_conexion_kernel = false;
        estado_conexion_con_cliente = false;
      } break;
      case -1: {
        log_info(logger, "el cliente se desconecto");
        // cliente_estado = CLIENTE_EXIT;
        estado_conexion_con_cliente = false;
        break;
      }
      default:
        log_warning(logger, "Operacion desconocida. No quieras meter la pata");
        break;
    }
  }
  pthread_exit(NULL);
}

void* escuchar_interrupt() {
  log_info(logger, "Iniciando escucha interrupt....");
  char* ip = config_get_string_value(config, "IP_ESCUCHA");
  char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
  int socket_cpu_dispatch = iniciar_servidor(ip, puerto);

  while (1) {
    int cliente_fd = esperar_cliente(socket_cpu_dispatch);
    cliente_status cliente_estado = CLIENTE_RUNNING;

    while (cliente_estado) {
      int cod_op = recibir_operacion(cliente_fd);
      switch (cod_op) {}
    }
  }

  free(ip);
  free(puerto);
}

void ciclo_instruccion(t_pcb* pcb, int socket_cliente) {
  log_info(logger, "Iniciando ciclo de instruccion");
  log_info(logger, "leyendo instrucciones");

  while (pcb->program_counter < list_size(pcb->instrucciones)) {
    t_instruccion* instruccion = malloc(sizeof(t_instruccion) + 1);
    instruccion = fetch(pcb);
    decode(instruccion, pcb, socket_cliente);
    // free(instruccion);
    pcb->program_counter++;
  }
}

t_instruccion* fetch(t_pcb* pcb) {
  return list_get(pcb->instrucciones, pcb->program_counter - 1);
}

void decode(t_instruccion* instruccion, t_pcb* pcb, int socket_cliente) {
  if (strcmp(instruccion->identificador, "NO_OP") == 0) {
    log_info(logger, "Ejecutando NO_OP...");
    // int retardo = (float)config_get_int_value(config, "RETARDO_NOOP") / (float)1000;
    int retardo = 1;
    sleep(retardo);
  }

  else if (strcmp(instruccion->identificador, "I/O") == 0) {
    log_info(logger, "Ejecutando IO...");
    // t_paquete* paquete_con_pcb = paquete_create();
    uint32_t tiempo_bloqueo = 0;
    pcb->program_counter++;
    tiempo_bloqueo = atoi(instruccion->params);
    // paquete_add_operacion_IO(paquete_con_pcb, pcb, tiempo_bloqueo);  //DESCOMENTAR PARA PROBAR LA RESPUESTA A KERNEL
    // enviar_pcb(pcb->socket, paquete_con_pcb);

    t_paquete* paquete = paquete_create();
    t_buffer* mensaje = crear_mensaje_pcb_actualizado(pcb, tiempo_bloqueo);

    paquete_cambiar_mensaje(paquete, mensaje);
    enviar_pcb_actualizado(socket_cliente, paquete);

    // paquete_destroy(paquete_con_pcb);
  }

  else if (strcmp(instruccion->identificador, "READ") ==
           0) { /// DE MEMORIA ANTICIPADAMENTE SE TIENE QUE TRAER EL TAM DE PAGINA PARA HACER LA TRADUCCION DESDE LA TLB
    log_info(logger, "Ejecutando READ...");
    int tam_pagina = 64; // TODO: ESTE NUMERO LO TIENE QUE TRAER DE MEMORIA. USAR SOLO PARA PRUEBAS
    int cant_entradas_por_tabla = 10;
    int num_pagina = (float)atoi(instruccion->params) / tam_pagina;
    uint32_t dir_logica = atoi(instruccion->params);
    log_info(logger, "Leyendo de TLB");
    bool acierto_tlb = esta_en_tlb(num_pagina);
    if (acierto_tlb == false) {
      // SE BUSCA EN MEMORIA LA PAGINA, PARA ELLO SE REALIZAN 3 ACCESOS:
      // ENVIO EL NUM DE TABLA DE 1er NIVEL JUNTO CON LA ENTRADA A DICHA TABLA
      // MEMORIA ME DEVUELVE EL NUM DE TABLA DE 2DO NIVEL
      // LUEGO ENVIO LA ENTRADA DE LA TABLA DE SEGUNDO NIVEL JUNTO CON EL NUM DE TABLA DE 2DO NIVEL
      // MEMORIA ME DEVUELVE EL NUM DE MARCO
      // CON ESTO ARMO LA DIRECCION FISICA Y ENVIO A MEMORIA PARA LEER EL DATO (DF= MARCO*TAM MARCO + DESPLAZAMIENTO)

      log_info(logger, "La pagina no se ecnuentra en la TLB, enviando solicitud a Memoria");

      // ACCESOS A MEMORIA PARA OBTENER EL MARCO
      // ACCESO PARA OBTENER TABLA SEGUNDO NIVEL
      t_solicitud_segunda_tabla* read = malloc(sizeof(t_solicitud_segunda_tabla));
      obtener_numero_tabla_segundo_nivel(read, pcb, num_pagina, cant_entradas_por_tabla);
      free(read);
      // RECIBO RESPUESTA DE MEMORIA
      t_paquete* paquete_respuesta = recibir_paquete(socket_memoria);
      t_respuesta_solicitud_segunda_tabla* respuesta_operacion = malloc(sizeof(t_respuesta_solicitud_segunda_tabla));
      respuesta_operacion = obtener_respuesta_read(paquete_respuesta);

      // Envio operacion para obtener el marco
      t_solicitud_marco* solicitud_marco = malloc(sizeof(t_solicitud_marco));
      obtener_numero_marco(
        solicitud_marco, num_pagina, cant_entradas_por_tabla, respuesta_operacion->num_tabla_segundo_nivel);
      free(solicitud_marco);
      free(respuesta_operacion);

      // RECIBO RESPUESTA DE MEMORIA
      t_paquete* paquete_respuesta_marco = recibir_paquete(socket_memoria);
      t_respuesta_solicitud_marco* respuesta_solicitud_marco =
        obtener_respuesta_solicitud_marco(paquete_respuesta_marco);
      free(respuesta_solicitud_marco);

      // ARMO SOLICITUD DATO
      t_solicitud_dato_fisico* solicitud_dato_fisico = malloc(sizeof(t_solicitud_dato_fisico));
      obtener_dato_fisico(
        solicitud_dato_fisico, respuesta_solicitud_marco->num_marco, num_pagina, tam_pagina, dir_logica);
      free(solicitud_dato_fisico);

      // RECIBO RESPUESTA DE MEMORIA
      t_paquete* paquete_respuesta_dato = recibir_paquete(socket_memoria);
      t_respuesta_dato_fisico* respuesta_solicitud_dato_fisico =
        obtener_respuesta_solicitud_dato_fisico(paquete_respuesta_dato);
      free(respuesta_solicitud_dato_fisico);
    } else {
      log_info(logger, "Accediendo a buscar el valor en memoria");
    }


  }

  else if (strcmp(instruccion->identificador, "WRITE") == 0) {
  }

  else if (strcmp(instruccion->identificador, "COPY") == 0) {
  }

  else if ((instruccion->identificador, "EXIT") == 0) {
    xlog(COLOR_CONEXION, "Ejecutando EXIT");
    pcb->program_counter++;
    t_paquete* paquete = paquete_create();
    t_buffer* mensaje = crear_mensaje_pcb_actualizado(pcb, NULL);

    paquete_cambiar_mensaje(paquete, mensaje);
    enviar_pcb_actualizado(socket_cliente, paquete);
  }
}

void obtener_dato_fisico(t_solicitud_dato_fisico* solicitud_dato_fisico,
                         int num_marco,
                         int num_pagina,
                         int tam_pagina,
                         uint32_t dir_logica) {
  armar_solicitud_dato_fisico(solicitud_dato_fisico, num_marco, num_pagina, tam_pagina, dir_logica);
  solicitud_dato_fisico->socket = socket_memoria;
  t_paquete* paquete_con_direccion_a_leer = paquete_create();
  paquete_add_solicitud_dato_fisico(paquete_con_direccion_a_leer, solicitud_dato_fisico);
  enviar_operacion_obtener_dato(socket_memoria, paquete_con_direccion_a_leer);
  paquete_destroy(paquete_con_direccion_a_leer);
}

void obtener_numero_marco(t_solicitud_marco* solicitud_marco,
                          int num_pagina,
                          int cant_entradas_por_tabla,
                          int numero_tabla_segundo_nivel) {
  armar_solicitud_marco(solicitud_marco, num_pagina, cant_entradas_por_tabla, numero_tabla_segundo_nivel);
  solicitud_marco->socket = socket_memoria;
  t_paquete* paquete_con_direccion_a_leer = paquete_create();
  paquete_add_solicitud_marco(paquete_con_direccion_a_leer, solicitud_marco);
  enviar_operacion_obtener_marco(socket_memoria, paquete_con_direccion_a_leer);
  paquete_destroy(paquete_con_direccion_a_leer);
}

void obtener_numero_tabla_segundo_nivel(t_solicitud_segunda_tabla* read,
                                        t_pcb* pcb,
                                        int num_pagina,
                                        int cant_entradas_por_tabla) {
  armar_solicitud_tabla_segundo_nivel(read, pcb->tabla_primer_nivel, num_pagina, cant_entradas_por_tabla);
  read->socket = socket_memoria;
  t_paquete* paquete_con_direccion_a_leer = paquete_create();
  paquete_add_solicitud_tabla_segundo_nivel(paquete_con_direccion_a_leer, read);
  enviar_operacion_obtener_segunda_tabla(socket_memoria, paquete_con_direccion_a_leer);
  paquete_destroy(paquete_con_direccion_a_leer);
}


void armar_solicitud_tabla_segundo_nivel(t_solicitud_segunda_tabla* solicitud_tabla_segundo_nivel,
                                         int num_tabla_primer_nivel,
                                         int num_pagina,
                                         int cant_entradas_por_tabla) {
  solicitud_tabla_segundo_nivel->num_tabla_primer_nivel = num_tabla_primer_nivel;
  solicitud_tabla_segundo_nivel->entrada_primer_nivel = (float)num_pagina / (float)cant_entradas_por_tabla;
}

void armar_solicitud_marco(t_solicitud_marco* solicitud_marco,
                           int num_pagina,
                           int cant_entradas_por_tabla,
                           int numero_tabla_segundo_nivel) {
  solicitud_marco->num_tabla_segundo_nivel = numero_tabla_segundo_nivel;
  solicitud_marco->entrada_segundo_nivel = num_pagina % cant_entradas_por_tabla;
}

void armar_operacion_read(t_operacion_read* read, t_instruccion* instruccion) {
  read->direccion_logica = atoi(instruccion->params);
}

void armar_solicitud_dato_fisico(t_solicitud_dato_fisico* solicitud_dato_fisico,
                                 int num_marco,
                                 int num_pagina,
                                 int tam_pagina,
                                 uint32_t dir_logica) {
  int desplazamiento = dir_logica - (num_pagina * tam_pagina);
  solicitud_dato_fisico->dir_fisica = num_marco * tam_pagina + desplazamiento;
}


int conectarse_a_memoria() {
  char* ip = config_get_string_value(config, "IP_MEMORIA");
  char* puerto = config_get_string_value(config, "PUERTO_MEMORIA");
  int fd_servidor = conectar_a_servidor(ip, puerto);

  if (fd_servidor == -1) {
    log_error(
      logger, "No se pudo establecer la conexión con CPU, inicie el servidor con %s e intente nuevamente", puerto);

    return -1;
  }

  return fd_servidor;
}

void iniciar_tlb() {
  tlb = list_create();
}

bool esta_en_tlb(int num_pagina) {
  bool es_la_pagina(t_entrada_tlb * contenido_tlb) {
    return contenido_tlb->pagina == num_pagina;
  }
  return list_any_satisfy(tlb, es_la_pagina);
}


void* iniciar_conexion_interrupt() {
  char* ip = config_get_string_value(config, "IP_ESCUCHA");
  char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");

  CONEXION_CPU_INTERRUPT = iniciar_servidor(ip, puerto); // TODO: evaluar posibilidad de condición de carrera

  xlog(COLOR_CONEXION, "Conexión Interrupt lista con el cliente Kernel");

  pthread_t th;
  pthread_create(&th, NULL, escuchar_conexiones_entrantes_en_interrupt, NULL), pthread_detach(th);

  pthread_exit(NULL);
}

void* escuchar_conexiones_entrantes_en_interrupt() {
  CONEXION_ESTADO estado_conexion_con_cliente = CONEXION_ESCUCHANDO;
  CONEXION_ESTADO ESTADO_CONEXION_INTERRUPT = CONEXION_ESCUCHANDO;

  while (ESTADO_CONEXION_INTERRUPT) {
    int socket_cliente = esperar_cliente(CONEXION_CPU_INTERRUPT);
    estado_conexion_con_cliente = CONEXION_ESCUCHANDO;

    while (estado_conexion_con_cliente) {
      int codigo_operacion = recibir_operacion(socket_cliente);

      switch (codigo_operacion) {
        case OPERACION_INTERRUPT: {
          t_paquete* paquete = recibir_paquete(socket_cliente);
          xlog(COLOR_PAQUETE, "se recibió una Interrupción");
          t_paquete* paquete_con_pcb = malloc(sizeof(t_paquete) + 1);
          paquete_con_pcb = recibir_paquete(socket_cliente);
          t_pcb* pcb_deserializado = paquete_obtener_pcb(paquete_con_pcb);

          pcb_deserializado->program_counter++;
          t_paquete* paquete_respuesta = paquete_create();
          t_buffer* mensaje = crear_mensaje_pcb_actualizado(pcb_deserializado, NULL);
          paquete_cambiar_mensaje(paquete_respuesta, mensaje);
          enviar_pcb_interrupt(socket_cliente, paquete_respuesta);

          paquete_destroy(paquete);
        } break;
        case OPERACION_MENSAJE: {
          recibir_mensaje(socket_cliente);
        } break;
        case OPERACION_EXIT: {
          xlog(COLOR_CONEXION, "Se recibió solicitud para finalizar ejecución");

          log_destroy(logger), close(CONEXION_CPU_INTERRUPT);
          // TODO: no estaría funcionando del todo, queda bloqueado en esperar_cliente()
          ESTADO_CONEXION_INTERRUPT = CONEXION_FINALIZADA;
          estado_conexion_con_cliente = CONEXION_FINALIZADA;
        } break;
        case -1: {
          xlog(COLOR_CONEXION, "el cliente se desconecto (socket=%d)", socket_cliente);

          // centinela para detener el loop del hilo asociado a la conexión
          // entrante
          estado_conexion_con_cliente = CONEXION_FINALIZADA;
        } break;
        default: { xlog(COLOR_ERROR, "Operacion %d desconocida", codigo_operacion); } break;
      }
    }
  }

  pthread_exit(NULL);
}