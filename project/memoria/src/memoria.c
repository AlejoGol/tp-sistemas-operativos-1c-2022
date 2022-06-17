#include "memoria.h"
#include "libstatic.h"
#include "serializado.h"
#include "utils-cliente.h"
#include "utils-servidor.h"
#include <commons/string.h>

void* reservar_memoria_inicial(int size_memoria_total) {
  void* memoria_total = malloc(size_memoria_total);

  memset(memoria_total, 0, size_memoria_total);

  return memoria_total;
}

void* escuchar_conexiones() {
  estado_conexion_memoria = true;
  char* ip = config_get_string_value(config, "IP_ESCUCHA");
  char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
  socket_memoria = iniciar_servidor(ip, puerto);

  while (estado_conexion_memoria) {
    int cliente_fd = esperar_cliente(socket_memoria);
    /*
        if (cliente_fd != -1) {
          t_paquete* paquete = paquete_create();
          t_buffer* mensaje = crear_mensaje("Conexión aceptada por MEMORIA");

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


// TODO: esto debería estar en swap.c
// por el momento sólo escuchamos las entradas de memoria,
void liberar_estructuras_en_swap() {
  xlog(COLOR_CONEXION, "SWAP recibió solicitud de Kernel para liberar recursos de un proceso");
}

void* manejar_nueva_conexion(void* args) {
  int socket_cliente = *(int*)args;
  estado_conexion_con_cliente = true;
  while (estado_conexion_con_cliente) {
    int codigo_operacion = recibir_operacion(socket_cliente);

    switch (codigo_operacion) {
      case OPERACION_MENSAJE: {
        recibir_mensaje(socket_cliente);

        // t_paquete* paquete = recibir_paquete(cliente_fd);
        // t_mensaje_handshake_cpu_memoria* mensaje = paquete_obtener_mensaje_handshake(paquete);

      } break;
      case READ: {
        /*log_info(logger, "Comenzando operacion READ...");
        t_paquete* paquete = recibir_paquete(socket_cliente);
        t_solicitud_segunda_tabla* read = paquete_obtener_solicitud_tabla_segundo_nivel(paquete);

        log_info(logger, "Paquete recibido...");

        // PROCESO EL VALOR ENVIADO POR CPU, POR AHORA HARDCODEO UN VALOR PARA PROBAR LA CONEXION

        t_respuesta_operacion_read* respuesta_read = malloc(sizeof(t_respuesta_operacion_read));
        respuesta_read->valor_buscado = 3;
        t_paquete* paquete_con_respuesta = paquete_create();
        paquete_add_respuesta_operacion_read(paquete_con_respuesta, respuesta_read);
        enviar_operacion_read(socket_cliente, paquete_con_respuesta);
        // DESCOMENTAR PARA RESOLVER SEG FAULT
        paquete_destroy(paquete_con_respuesta);

        free(respuesta_read);*/
        break;
      }
      /*
      case OPERACION_INICIALIZAR_ESTRUCTURAS: {
        t_paquete* paquete = recibir_paquete(socket_cliente);
        t_pcb* pcb = paquete_obtener_pcb(paquete);
        paquete_destroy(paquete);

        // TODO: resolver cuando se avance el módulo..

        xlog(COLOR_CONEXION, "Se recibió solicitud de Kernel para inicializar estructuras de un proceso");

        pcb->tabla_primer_nivel = 1;
        t_paquete* paquete_con_pcb_actualizado = paquete_create();
        paquete_add_pcb(paquete_con_pcb_actualizado, pcb);


        // TODO: deberia agregar al pcb el valor de la tabla de paginas
        confirmar_estructuras_en_memoria(socket_cliente, paquete_con_pcb_actualizado);
        paquete_destroy(paquete_con_pcb_actualizado);
      } break;
      */
      case OPERACION_EXIT: {
        xlog(COLOR_CONEXION, "Se recibió solicitud para finalizar ejecución");

        log_destroy(logger), close(socket_memoria);
        // TODO: no estaría funcionando del todo, queda bloqueado en esperar_cliente()
        estado_conexion_memoria = false;
        estado_conexion_con_cliente = false;

        break;
      }
      case OPERACION_OBTENER_SEGUNDA_TABLA: {
        xlog(COLOR_CONEXION, "Obteniendo numero de tabla de segundo nivel");
        // codigo_operacion = recibir_operacion(socket_memoria);
        t_paquete* paquete = recibir_paquete(socket_cliente);
        t_solicitud_segunda_tabla* req = malloc(sizeof(t_solicitud_segunda_tabla));

        req = obtener_solicitud_tabla_segundo_nivel(paquete);

        /// HACER LOS LLAMADOS A LOS METODOS CORRESPONDIENTES PARA OBTENER EL NUM DE TABLA
        // int numero_tabla = buscar_tabla_segundo_nivel(req->num_tabla_primer_nivel, req->entrada_primer_nivel);
        int numero_TP_segundo_nivel = obtener_numero_TP_segundo_nivel(req->num_tabla_primer_nivel, req->entrada_primer_nivel);

        xlog(COLOR_INFO, "SEGUNDA TABLA: %d", numero_TP_segundo_nivel);

        // TODO: validar si no hay una función que agregue el contenido más fácil ó crear una abstracción
        t_paquete* paquete_respuesta = paquete_create();
        t_respuesta_solicitud_segunda_tabla* resp = malloc(sizeof(t_respuesta_solicitud_segunda_tabla));
        resp->socket = socket_cliente;
        resp->num_tabla_segundo_nivel = numero_TP_segundo_nivel;

        t_buffer* mensaje = crear_mensaje_respuesta_segunda_tabla(resp);
        paquete_cambiar_mensaje(paquete_respuesta, mensaje),
          enviar_operacion_respuesta_segunda_tabla(socket_cliente, paquete_respuesta);

        free(paquete_respuesta);
        paquete_destroy(paquete);

        break;
      }
      case OPERACION_OBTENER_MARCO: {
        xlog(COLOR_CONEXION, "Obteniendo numero de marco");
        // codigo_operacion = recibir_operacion(socket_memoria);
        t_paquete* paquete = recibir_paquete(socket_cliente);
        t_solicitud_marco* req = malloc(sizeof(t_solicitud_marco));

        req = obtener_solicitud_marco(paquete);


        int num_marco = obtener_marco(req->num_tabla_segundo_nivel, req->entrada_segundo_nivel, req->operacion);
        xlog(COLOR_INFO, "NUMERO MARCO: %d", num_marco);


        t_paquete* paquete_respuesta = paquete_create();
        t_respuesta_solicitud_marco* resp = malloc(sizeof(t_respuesta_solicitud_marco));
        resp->num_marco = num_marco;
        t_buffer* mensaje = crear_mensaje_respuesta_marco(resp);
        paquete_cambiar_mensaje(paquete_respuesta, mensaje),
          enviar_operacion_obtener_marco(socket_cliente, paquete_respuesta);

        free(paquete_respuesta);
        free(paquete);
        break;
      }
      case OPERACION_OBTENER_DATO: {
        xlog(COLOR_CONEXION, "Obteniendo dato fisico en memoria");
        // codigo_operacion = recibir_operacion(socket_memoria);
        t_paquete* paquete = recibir_paquete(socket_cliente);
        t_solicitud_dato_fisico* req = malloc(sizeof(t_solicitud_dato_fisico));

        req = obtener_solicitud_dato(paquete);

        uint32_t direccion_fisica = req->dir_fisica;

        uint32_t dato_buscado = 0;
        dato_buscado = buscar_dato_en_memoria(direccion_fisica);
        xlog(COLOR_INFO, "DATO BUSCADO: %d", dato_buscado);

        /// HACER LOS LLAMADOS A LOS METODOS CORRESPONDIENTES PARA OBTENER EL NUM DE TABLA

        t_paquete* paquete_respuesta = paquete_create();
        t_respuesta_dato_fisico* resp = malloc(sizeof(t_respuesta_dato_fisico));
        // resp->size_dato = 6;
        // resp->dato_buscado = malloc(7);
        memcpy(&(resp->dato_buscado), &dato_buscado, sizeof(uint32_t));
        // memcpy(resp->dato_buscado, "holass", 7);
        // memcpy(resp->dato_buscado + 6, "\0", 1);
        t_buffer* mensaje = crear_mensaje_respuesta_dato_fisico(resp);
        paquete_cambiar_mensaje(paquete_respuesta, mensaje),
          enviar_operacion_obtener_dato(socket_cliente, paquete_respuesta);

        // free(dato_buscado);
        free(paquete_respuesta);

        break;
      }

      case OPERACION_ESCRIBIR_DATO: {
        xlog(COLOR_CONEXION, "Escribiendo dato en memoria");
        // codigo_operacion = recibir_operacion(socket_memoria);
        t_paquete* paquete = recibir_paquete(socket_cliente);
        t_escritura_dato_fisico* req = malloc(sizeof(t_escritura_dato_fisico));

        req = obtener_solicitud_escritura_dato(paquete);

        uint32_t dir_fisica = req->dir_fisica;
        uint32_t valor = req->valor;

        int resultado_escritura = escribir_dato(dir_fisica, valor);


        t_paquete* paquete_respuesta = paquete_create();
        t_respuesta_escritura_dato_fisico* resp = malloc(sizeof(t_respuesta_escritura_dato_fisico));
        resp->resultado = 1;
        t_buffer* mensaje = crear_mensaje_respuesta_escritura_dato_fisico(resp);
        paquete_cambiar_mensaje(paquete_respuesta, mensaje),
          enviar_operacion_escribir_dato(socket_cliente, paquete_respuesta);

        free(paquete_respuesta);

        break;
      }
      case OPERACION_PROCESO_SUSPENDIDO: {
        t_paquete* paquete = recibir_paquete(socket_cliente);
        // TODO: resolver cuando se avance el módulo..

        xlog(COLOR_CONEXION, "Se recibió solicitud de Kernel para suspender proceso");
        confirmar_suspension_de_proceso(socket_cliente, paquete);
        paquete_destroy(paquete);
      } break;
      case OPERACION_INICIALIZAR_ESTRUCTURAS: {
        t_paquete* paquete = recibir_paquete(socket_cliente);
        t_pcb* pcb = paquete_obtener_pcb(paquete);
        paquete_destroy(paquete);

        // TODO: resolver cuando se avance el módulo..

        xlog(COLOR_CONEXION, "Se recibió solicitud de Kernel para inicializar estructuras de un proceso");

        pcb->tabla_primer_nivel = 1;
        t_paquete* paquete_con_pcb_actualizado = paquete_create();
        paquete_add_pcb(paquete_con_pcb_actualizado, pcb);


        // TODO: deberia agregar al pcb el valor de la tabla de paginas
        confirmar_estructuras_en_memoria(socket_cliente, paquete_con_pcb_actualizado);
        paquete_destroy(paquete_con_pcb_actualizado);
      } break;
      case OPERACION_PROCESO_FINALIZADO: {
        t_paquete* paquete = recibir_paquete(socket_cliente);
        // TODO: resolver cuando se avance el módulo de memoria
        liberar_estructuras_en_swap();

        xlog(COLOR_CONEXION, "Memoria/Swap recibió solicitud de Kernel para liberar las estructuras de un proceso");
        paquete_destroy(paquete);
      } break;
      case -1: {
        log_info(logger, "el cliente se desconecto");
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

int escribir_dato(uint32_t dir_fisica, uint32_t valor) {
  // Busco a que proceso pertenece

  int resultado = 0;

  return resultado;
}

int buscar_marco(int num_tabla_segundo_nivel, int entrada_segundo_nivel) {
  int marco = 0;

  int es_la_tabla(t_tabla_segundo_nivel * tabla_actual) {
    return tabla_actual->num_tabla == num_tabla_segundo_nivel;
  }

  t_tabla_segundo_nivel* tabla_segundo_nivel = list_find(lista_tablas_segundo_nivel, (void*)es_la_tabla);
  t_entrada_tabla_segundo_nivel* entrada = (t_entrada_tabla_segundo_nivel*) list_get(tabla_segundo_nivel->entradas, entrada_segundo_nivel);
  entrada->bit_uso = 1;
  marco = entrada->num_marco;

  return marco;
}

bool marcos_disponibles_para_proceso(int pid) {
  int es_el_proceso(int proc) {
    return proc == pid;
  }

  int cant_marcos_asignados_a_proceso = list_count_satisfying(tabla_marcos, (void*)es_el_proceso);

  return cant_marcos_asignados_a_proceso;
}

int asignar_marco_libre_o_reemplazar_pagina(int num_tabla_segundo_nivel, int entrada_segundo_nivel) {
  int marco = 0;

  int es_la_tabla(t_tabla_segundo_nivel * tabla_actual) {
    return tabla_actual->num_tabla == num_tabla_segundo_nivel;
  }

  t_tabla_segundo_nivel* tabla_segundo_nivel = list_find(lista_tablas_segundo_nivel, (void*)es_la_tabla);

  t_entrada_tabla_segundo_nivel* entrada =
    (t_entrada_tabla_segundo_nivel*)list_get(tabla_segundo_nivel->entradas, entrada_segundo_nivel);
  // entrada->bit_uso = 1;
  // marco = entrada->num_marco;*/

  if (entrada->num_marco != -1) {
    marco = entrada->num_marco;

    if (strcmp(algoritmo_reemplazo, "CLOCK-M") == 0) {
      entrada->bit_modif = 1;
    }
    entrada->bit_uso = 1;
  } else if (!marcos_disponibles_para_proceso(tabla_segundo_nivel->pid)) {
    t_entrada_tabla_segundo_nivel* entrada_victima = ejecutar_reemplazo(tabla_segundo_nivel->pid, entrada);
  } else {
    marco = buscar_marco_libre();
  }

  return marco;
}

int obtener_marco(int num_tabla_segundo_nivel, int entrada_segundo_nivel, int operacion) {
  int marco = entrada_TP_2do_nivel_marco_asignado(num_tabla_segundo_nivel, entrada_segundo_nivel);

  /*

  int marco = 0;
  if (operacion == 1) {
    marco = buscar_marco(num_tabla_segundo_nivel, entrada_segundo_nivel);

  } else {
    marco = asignar_marco_libre_o_reemplazar_pagina(num_tabla_segundo_nivel, entrada_segundo_nivel);
  }
   */
  // 1: operacion lectura
  // 2: operacion de escritura

  return marco;
}

// TODO: implementar
int entrada_TP_2do_nivel_marco_asignado(int num_tabla_segundo_nivel, int entrada_segundo_nivel) {
}

int obtener_numero_TP_segundo_nivel(int numero_TP_primer_nivel, int numero_entrada_TP_primer_nivel) {
  t_tabla_primer_nivel* TP_primer_nivel = (t_tabla_primer_nivel*) dictionary_get(tablas_de_paginas_primer_nivel, string_itoa(numero_TP_primer_nivel));

  // no se estaba obteniendo la entrada como se quería, el list_get trabaja por posición y se pasaba el valor de un atributo de la estructura no de la posición en la lista
  // t_entrada_tabla_primer_nivel* entrada_primer_nivel = (t_entrada_tabla_primer_nivel*) list_get(TP_primer_nivel->entradas_primer_nivel, entrada_tabla);
  t_entrada_tabla_primer_nivel* entrada_primer_nivel = (t_entrada_tabla_primer_nivel*) dictionary_get(TP_primer_nivel->entradas_primer_nivel, string_itoa(numero_entrada_TP_primer_nivel));

  return entrada_primer_nivel->num_tabla_segundo_nivel;
}

uint32_t buscar_dato_en_memoria(uint32_t dir_fisica) {
  xlog(COLOR_CONEXION, "Buscando en memoria la dir fisica: %d", dir_fisica);

  uint32_t dato_buscado = 0;
  int num_marco_buscado = dir_fisica / tam_marcos;
  int desplazamiento = dir_fisica % tam_marcos;

  bool es_el_marco(t_marco * marco) {
    return (marco->num_marco == num_marco_buscado);
  }

  t_marco* marco = list_find(tabla_marcos, (void*)es_el_marco);
  int inicio = (num_marco_buscado * tam_marcos) + desplazamiento;
  memcpy(&dato_buscado, memoria_principal + inicio, sizeof(int));
  return dato_buscado;
}

int inicializar_tabla_marcos() {
  xlog(COLOR_CONEXION, "Inicializando tabla de marcos");
  int tam_tabla = 0;

  cant_marcos = (size_memoria_principal) / tam_marcos;

  while (tam_tabla < cant_marcos) {
    t_marco* marco = malloc(sizeof(t_marco));
    marco->num_marco = tam_tabla;
    marco->direccion = 0;
    marco->pid = 0;
    marco->ocupado = 0;
    list_add(tabla_marcos, marco);
    tam_tabla++;
  }
  return tam_tabla;
}

int obtener_cantidad_entradas_por_tabla_por_config(){
  return config_get_int_value(config, "ENTRADAS_POR_TABLA");
}

void inicializar_estructuras_de_este_proceso(int pid, int tam_proceso) {
  // TODO: validar el comentario de abajo
  /// ESTA FUNCION DEBE DEVOLVER EL NUM DE TABLA DE PRIMER NIVEL ASIGNADA
  xlog(COLOR_CONEXION, "Inicializando proceso");

  t_tabla_primer_nivel* tabla_primer_nivel = tabla_paginas_primer_nivel_create();

  // agregamos una TP_primer_nivel en una estructura global
  dictionary_put(tablas_de_paginas_primer_nivel, string_itoa(tabla_primer_nivel->num_tabla), tabla_primer_nivel);

  /*
  // TODO: evaluar si remover el tamaño_acumulado, se había pensado para una asignación dinámica de frames
  int tam_acumulado = 0;
  int cant_marcos_asignados = 0;
  int marcos_por_proceso = config_get_int_value(config, "MARCOS_POR_PROCESO");

  // TODO: generar abstraccion crear_tabla_paginas_primer_nivel() y inicializar_tabla_paginas_primer_nivel()
  t_tabla_primer_nivel* tabla_primer_nivel = malloc(sizeof(t_tabla_primer_nivel));

  // TODO: usar una variable global cantidad_tabla_paginas_primer nivel, usar ese valor e incrementar
  // validar si conviene usar otra manera
  tabla_primer_nivel->num_tabla = 1;

  // TODO: evaluar si remover el identificador del proceso
  tabla_primer_nivel->pid = pid;

  // es más fácil acceder con el diccionario por el numero de entrada, evitando iterar sobre la lista preguntando po numero de entrada
  // tabla_primer_nivel->entradas = list_create();
  tabla_primer_nivel->entradas_primer_nivel = dictionary_create();

  // agrega a la TP_primer_nivel tantas entradas como se diga por config
  // cada entrada representa una TP_segundo_nivel
  for (int i = 0; i < obtener_cantidad_entradas_por_tabla_por_config(); i++) {
    t_entrada_tabla_primer_nivel* entrada_primer_nivel = malloc(sizeof(t_entrada_tabla_primer_nivel));

    // TODO: esto debería cambiar, sería suficiente con usar el contador del for?
    // esto identifica cada entrada de TP 1er nivel, la MMU accede a ésta usando floor(numero_pagina_DL/cant_entradas_por_tabla)
    entrada_primer_nivel->entrada_primer_nivel = 1;

    // TODO: desacoplar y generar abstracción agregar_tabla_paginas_segundo_nivel()
    t_tabla_segundo_nivel* tabla_paginas_segundo_nivel = malloc(sizeof(t_tabla_segundo_nivel));

    // TODO: evaluar si remover el identificador del proceso
    tabla_paginas_segundo_nivel->pid = pid;

    // TODO: esto debe coincidir con num_tabla_segundo_nivel que tiene la entrada de la TP de primer nivel
    tabla_paginas_segundo_nivel->num_tabla = 2;

    // cada entrada es del tipo (numero_entrada_TP_primer_nivel, numero_TP_segundo_nivel)
    tabla_paginas_segundo_nivel->entradas_segundo_nivel = dictionary_create();

    // agrega a la TP_segundo_nivel tantas entradas como se diga por config
    // cada entrada representa una entrada de la TP_segundo_nivel
    for (int j = 0; j < obtener_cantidad_entradas_por_tabla_por_config(); j++) {
      // TODO: delegar y generar una abstracción validar_marcos_asignados() ò similar
      // TODO: validar si es necesaria esta validación, porque ahora los marcos se inicializaron en -1
      if (cant_marcos_asignados <= marcos_por_proceso) {
        t_entrada_tabla_segundo_nivel* entrada_tabla_segundo_nivel = malloc(sizeof(t_entrada_tabla_segundo_nivel));

        // TODO: validar en el foro si está ok definirlo asi el valor
        entrada_tabla_segundo_nivel->entrada_segundo_nivel = j;

        // TODO: delegar, desacoplar y generar abstracción inicializar_tabla_paginas()
        entrada_tabla_segundo_nivel->num_marco = -1; // valor negativo porque no tiene un marco asignado

        entrada_tabla_segundo_nivel->bit_uso = 0;
        entrada_tabla_segundo_nivel->bit_modif = 0;
        entrada_tabla_segundo_nivel->bit_presencia = 0;

        dictionary_put(tabla_paginas_segundo_nivel->entradas_segundo_nivel, string_itoa(entrada_tabla_segundo_nivel->entrada_segundo_nivel) , entrada_tabla_segundo_nivel);
        cant_marcos_asignados++;

        // TODO: evaluar si remover, se usaba porque se consideraba una asignación dinámica de frames
        // tam_proceso += tam_marcos;
      }
    }
    entrada_primer_nivel->num_tabla_segundo_nivel = tabla_paginas_segundo_nivel->num_tabla;

    // agregamos una entrada_primer_nivel a la TP_primer_nivel
    dictionary_put(tabla_primer_nivel->entradas_primer_nivel, string_itoa(entrada_primer_nivel->entrada_primer_nivel), entrada_primer_nivel);

    // agregamos una TP_segundo_nivel en una estructura global
    dictionary_put(tablas_de_paginas_segundo_nivel, string_itoa(tabla_paginas_segundo_nivel->num_tabla) , tabla_paginas_segundo_nivel);
  }

  // TODO: (???)
  // dictionary_put(diccionario_paginas, string_itoa(pid), tabla_primer_nivel);
  // COMENTAR ESTO Y DESCOMENTAR LA DE ARRIBA: SOLO PARA PRUEBAS
  dictionary_put(tablas_de_paginas_primer_nivel, string_itoa(tabla_primer_nivel->num_tabla), tabla_primer_nivel);
  */
}

int generar_numero_tabla() {
  srand(time(NULL));
  int r = rand();
  return r;
}

int buscar_marco_libre() {
  int buscar_primer_libre(t_marco * marco) {
    return marco->ocupado == 0;
  }

  t_marco* marco_libre = (t_marco*)list_find(tabla_marcos, (void*)buscar_primer_libre);
  marco_libre->ocupado = 1;
  return marco_libre->num_marco;
}


void mostrar_tabla_marcos() {
  xlog(COLOR_CONEXION, "########TABLA MARCOS################");


  for (int i = 0; i < list_size(tabla_marcos); i++) {
    t_marco* marco = (t_marco*)list_get(tabla_marcos, i);
    printf("Num Marco: %d ", marco->num_marco);
    printf("Direccion: %d ", marco->direccion);
    printf("PID: %d\n", marco->pid);
  }
}


void llenar_memoria_mock() {
  int offset = 0;
  int num_marco = 0;
  // while (offset < size_memoria_principal) {
  memset(memoria_principal + offset, 5, size_memoria_principal);
  offset = offset + 64;
  num_marco += 1;
  //}
}

// TODO: validar si está ok, y en donde se usa
t_tabla_primer_nivel* encontrar_tabla(int pid) {
  int i = 0;
  t_tabla_primer_nivel* tabla = malloc(sizeof(t_tabla_primer_nivel));
  while (i < dictionary_size(tablas_de_paginas_primer_nivel)) {
    tabla = dictionary_get(tablas_de_paginas_primer_nivel, string_itoa(i));
    if (tabla->pid == pid) {
      return tabla;
    }
    i++;
  }
  return NULL;
}

// TODO: validar, se hicieron cambios en la tp_2do_nivel, ahora es diccionario
void encontrar_marcos_en_tabla_segundo_nivel(int num_tabla_segundo_nivel, t_list* marcos) {
  t_tabla_segundo_nivel* tabla_segundo_nivel = list_get(lista_tablas_segundo_nivel, num_tabla_segundo_nivel);


  for (int i = 0; i < list_size(tabla_segundo_nivel->entradas); i++) {
    t_entrada_tabla_segundo_nivel* entrada = list_get(tabla_segundo_nivel->entradas, i);

    if (entrada->num_marco != -1) {
      t_marco_asignado* marco_asignado = malloc(sizeof(marco_asignado));
      marco_asignado->marco = entrada->num_marco;
      marco_asignado->entrada;
      list_add(marcos, marco_asignado);
    }
  }
}

// TODO: validar, se hicieron cambios en la tp_primer_nivel, las entradas son un diccionario
t_list* encontrar_marcos_asignados_a_proceso(int pid) {
  t_list* marcos = list_create();
  // ITERO LAS TABLAS DE PAGINAS

  t_tabla_primer_nivel* tabla = encontrar_tabla(pid);

  for (int i = 0; i < list_size(tabla->entradas); i++) {
    t_entrada_tabla_primer_nivel* entrada_primer_nivel = list_get(tabla->entradas, i);

    if (entrada_primer_nivel->num_tabla_segundo_nivel != NULL) {
      encontrar_marcos_en_tabla_segundo_nivel(entrada_primer_nivel->num_tabla_segundo_nivel, marcos);
    }
  }
}

int ejecutar_reemplazo(int pid, t_entrada_tabla_segundo_nivel* entrada) {
  char* algoritmo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
  t_list* marcos_involucrados = encontrar_marcos_asignados_a_proceso(pid);
  t_entrada_tabla_segundo_nivel* entrada_victima = malloc(sizeof(t_entrada_tabla_segundo_nivel));
  if (strcmp(algoritmo, "CLOCK")) {
    entrada_victima = ejecutar_clock(marcos_involucrados, entrada);

    // TODO: REALIZAR EL REEMPLAZO ENTRE LA ENTRADA A REEMPLAZAR Y LA ENTRADA REFERENCIADA

  } else {
    // marco_victima = ejecutar_clock_modificado(marcos_involucrados, entrada);
  }

  return 0;
}

t_entrada_tabla_segundo_nivel* ejecutar_clock(t_list* marcos, t_entrada_tabla_segundo_nivel* entrada) {
  t_entrada_tabla_segundo_nivel* entrada_victima = malloc(sizeof(t_entrada_tabla_segundo_nivel));

  while (puntero_clock < list_size(marcos)) { // Recorro hasta el size de la lista
    t_marco_asignado* marco_asignado_a_entrada = list_get(marcos, puntero_clock);
    if (marco_asignado_a_entrada->entrada->bit_uso != 0) {
      marco_asignado_a_entrada->entrada->bit_uso = 0;
    } else {
      entrada_victima = marco_asignado_a_entrada->entrada;
      puntero_clock++;
      break;
    }

    if (puntero_clock + 1 == list_size(marcos)) {
      puntero_clock = 0;
      continue;
    }
    puntero_clock++;
  }

  return entrada_victima;
}

t_tabla_primer_nivel* tabla_paginas_primer_nivel_create(){
  t_tabla_primer_nivel* tabla_paginas_primer_nivel = malloc(sizeof(t_tabla_primer_nivel));

  // TODO: usar una variable global cantidad_tabla_paginas_primer nivel, usar ese valor e incrementar
  // validar si conviene usar otra manera
  // ó contar la cantidad de elementos en la estructura global (en el diccionario)
  tabla_paginas_primer_nivel->num_tabla = 1;

  tabla_paginas_primer_nivel->entradas_primer_nivel = dictionary_create();

  for (int numero_entrada_primer_nivel = 0; i < obtener_cantidad_entradas_por_tabla_por_config(); numero_entrada_primer_nivel++) {
    t_entrada_tabla_primer_nivel* entrada_primer_nivel = malloc(sizeof(t_entrada_tabla_primer_nivel));

    // esto identifica cada entrada de TP 1er nivel, la MMU accede a ésta usando
    // floor(numero_pagina_DL/cant_entradas_por_tabla)
    entrada_primer_nivel->entrada_primer_nivel = numero_entrada_primer_nivel;

    // TODO: validar si se debe usar otro criterio para el numero_tabla_segundo_nivel
    t_tabla_segundo_nivel* tabla_paginas_segundo_nivel = tabla_paginas_segundo_nivel_create(numero_entrada_primer_nivel);
    dictionary_put(tablas_de_paginas_segundo_nivel,
                   string_itoa(tabla_paginas_segundo_nivel->num_tabla),
                   tabla_paginas_segundo_nivel);


    entrada_primer_nivel->num_tabla_segundo_nivel = tabla_paginas_segundo_nivel->num_tabla;

    // agregamos una entrada_primer_nivel a la TP_primer_nivel
    dictionary_put(tabla_primer_nivel->entradas_primer_nivel,
                   string_itoa(entrada_primer_nivel->entrada_primer_nivel),
                   entrada_primer_nivel);
  }

  return tabla_paginas_primer_nivel;
}

void inicializar_entrada_de_tabla_paginas(t_entrada_tabla_segundo_nivel entrada_tabla_segundo_nivel) {
  entrada_tabla_segundo_nivel->bit_uso = 0;
  entrada_tabla_segundo_nivel->bit_modif = 0;
  entrada_tabla_segundo_nivel->bit_presencia = 0;

  entrada_tabla_segundo_nivel->num_marco = -1; // valor negativo porque no tiene un marco asignado
}

t_tabla_segundo_nivel* tabla_paginas_segundo_nivel_create(int numero_tabla_segundo_nivel) {
  t_tabla_segundo_nivel* tabla_paginas_segundo_nivel = malloc(sizeof(t_tabla_segundo_nivel));

  // TODO: esto debe coincidir con num_tabla_segundo_nivel que tiene la entrada de la TP de primer nivel
  tabla_paginas_segundo_nivel->num_tabla = numero_tabla_segundo_nivel;

  for (int numero_entrada_segundo_nivel = 0; i < obtener_cantidad_entradas_por_tabla_por_config(); numero_entrada_segundo_nivel++) {
    t_entrada_tabla_segundo_nivel* entrada_tabla_segundo_nivel = malloc(sizeof(t_entrada_tabla_segundo_nivel));
    entrada_tabla_segundo_nivel->entrada_segundo_nivel = numero_entrada_segundo_nivel;

    inicializar_entrada_de_tabla_paginas(entrada_tabla_segundo_nivel);

    // agregamos una TP_segundo_nivel en una estructura global
    dictionary_put(tabla_paginas_segundo_nivel->entradas_segundo_nivel,
                   string_itoa(entrada_tabla_segundo_nivel->entrada_segundo_nivel),
                   entrada_tabla_segundo_nivel);
  }

  return tabla_paginas_segundo_nivel;
}
