#ifndef __MY_STATIC_LIB_H__
#define __MY_STATIC_LIB_H__

#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "xlog.h"
#include "timer.h"

typedef enum {
  OPERACION_EXIT=0,
  MENSAJE = 1,
  PAQUETE = 2,
  CONSOLA = 3,
  PCB = 4,
  INTERRUPT = 5,

  // TODO: el resto deben ser removidos
  OPERACION_MENSAJE, OPERACION_PAQUETE, OPERACION_PCB, OPERACION_PCB_DESALOJADO,
  OPERACION_INTERRUPT, OPERACION_CONSOLA,
  OPERACION_PCB_CON_IO,
  PAQUETE_INSTRUCCION
  } op_code;

typedef enum { CONEXION_FINALIZADA = 0, CONEXION_ESCUCHANDO = 1 } CONEXION_ESTADO;

// TODO: pendiente de remover, hasta que cpu implemente CONEXION_ESTADO
typedef enum { CLIENTE_EXIT = 0, CLIENTE_RUNNING = 1 } cliente_status;

typedef struct {
  int size;
  void* stream;
} t_buffer;

typedef struct {
  op_code codigo_operacion;
  t_buffer* buffer;
} t_paquete;

// TODO: definir si se requiere esta abstracción
typedef struct{
  char* identificador;
  char* params;
} t_instruccion;

typedef enum {
  NEW,
  READY,
  RUNNING,
  BLOCKED,
  SUSBLOCKED,
  SUSREADY,
  FINISHED
} t_pcb_estado;

// TODO: definir atributos: instrucciones y tabla de paginas
typedef struct {
  int socket;
  int pid;
  int tamanio;
  int estimacion_rafaga;
  int tiempo_en_ejecucion;
  int tiempo_de_bloqueado;
  int program_counter;
  t_pcb_estado estado;
  t_list* instrucciones;
} t_pcb;

t_config* iniciar_config(char*);
t_log* iniciar_logger(char* archivo, char* nombre);

t_buffer* crear_mensaje(char* texto);
t_pcb* pcb_create(int socket, int pid, int tamanio);
t_instruccion* instruccion_create(char* identificador, char* params);
t_paquete* paquete_create();
t_paquete* paquete_instruccion_create(int tamanio);
t_buffer* empty_buffer();
int get_paquete_size(t_paquete* paquete);

void paquete_add_mensaje(t_paquete* paquete, t_buffer* nuevo_mensaje);

void mensaje_destroy(t_buffer* mensaje);
void paquete_destroy(t_paquete* paquete);
void instruccion_destroy(t_instruccion* instruccion);

void liberar_conexion(int socket);
void terminar_programa(int conexion, t_log* logger, t_config* config);
void paquete_cambiar_mensaje(t_paquete* paquete, t_buffer* mensaje);
void asignar_codigo_operacion(op_code codigo_operacion, t_paquete* paquete);

void imprimir_instrucciones(t_list* lista);
void imprimir_instruccion(t_instruccion* instruccion);
void imprimir_pcb(t_pcb* pcb);
void pcb_destroy(t_pcb* pcb);
t_pcb* pcb_fake();
#endif
