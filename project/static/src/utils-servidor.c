#include "utils-servidor.h"

int iniciar_servidor(char* ip, char* puerto) {
  int socket_servidor;

  struct addrinfo hints, *servinfo;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(ip, puerto, &hints, &servinfo);

  socket_servidor =
    socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

  listen(socket_servidor, SOMAXCONN);

  freeaddrinfo(servinfo);

  return socket_servidor;
}

int esperar_cliente(int socket_servidor) {
  struct sockaddr dir_cliente;
  socklen_t tam_direccion = sizeof(struct sockaddr);

  int socket_cliente = accept(socket_servidor, &dir_cliente, &tam_direccion);

  /* log_info(logger, "Se conecto un cliente!"); */

  return socket_cliente;
}

int recibir_operacion(int socket_cliente) {
  int cod_op;
  if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) != 0)
    return cod_op;
  else {
    close(socket_cliente);
    return -1;
  }
}

void* recibir_buffer(int* size, int socket_cliente) {
  void* buffer;

  recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
  buffer = malloc(*size);
  recv(socket_cliente, buffer, *size, MSG_WAITALL);

  return buffer;
}

void recibir_mensaje(int socket_cliente) {
  int size;
  char* buffer = recibir_buffer(&size, socket_cliente);

  free(buffer);
}

t_list* recibir_paquete(int socket_cliente) {
  int size;
  int desplazamiento = 0;
  void* buffer;
  t_list* valores = list_create();
  int tamanio;

  buffer = recibir_buffer(&size, socket_cliente);
  while (desplazamiento < size) {
    memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);
    char* valor = malloc(tamanio);
    memcpy(valor, buffer + desplazamiento, tamanio);
    desplazamiento += tamanio;
    list_add(valores, valor);
  }
  free(buffer);
  return valores;
}