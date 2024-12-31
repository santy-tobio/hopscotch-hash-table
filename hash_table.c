#include "hash_table.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_CAPACITY 32
#define HOP_RANGE 32
#define SEED 42
#define FULL_NEIGHBORHOOD 0xFFFFFFFF
#define ROTL32(x, r) ((x << r) | (x >> (32 - r)))

typedef struct Nodo
{
    char *key;
    void *value;
} Nodo;

struct dictionary 
{
    destroy_f destroy;
    size_t capacity; // cantidad de elementos que puede tener la tabla
    size_t size; // cantidad de elementos en la tabla
    Nodo *nodes;
    unsigned int *bitmaps;
    double alpha;
};

char* strdup(const char* string) 
{
  size_t len = strlen(string);
  char* p = malloc(len + 1);
  if(!p) return NULL; //si no se pudo asignar memoria
  strcpy(p, string);
  return p;
}
void update_alpha(dictionary_t *dictionary) 
{
  dictionary->alpha = (double) (dictionary->size) / (double) (dictionary->capacity);
}
size_t murmur_hash(const char *key, int len, int seed) {
    const uint8_t *data = (const uint8_t *)key;
    const int nblocks = len / 4;

    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    // Body
    const uint32_t *blocks = (const uint32_t *)(data + nblocks * 4);
    for (int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i];

        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    // Tail
    const uint8_t *tail = (const uint8_t *)(data + nblocks * 4);
    uint32_t k1 = 0;

    switch (len & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
                k1 *= c1;
                k1 = ROTL32(k1, 15);
                k1 *= c2;
                h1 ^= k1;
    }

    // Finalization
    h1 ^= len;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}
size_t get_hash_index(dictionary_t *dictionary,const char * key) 
{
  if(!key) return 0; //si no hay key, devuelvo 0 (no deberia pasar nunca)
  int len = (int) (strlen(key) + 1); // Calcular la longitud de la cadena clave, +1 es para incluir el \0
  size_t hash = murmur_hash(key, len , SEED);
  return hash % dictionary->capacity;
}
bool replace_value(dictionary_t *dictionary, size_t index, void *value) 
{
  if(dictionary->destroy) dictionary->destroy(dictionary->nodes[index].value); // si existe destroy, la uso
  dictionary->nodes[index].value = value;
  return true;
}
void update_bitmap(dictionary_t *dictionary, const char *key, size_t index, bool value) 
{
  if(!key) return; //si no hay key, no hago nada
  size_t bitmap_to_update = get_hash_index(dictionary,key);
  size_t bit_to_update = (index - bitmap_to_update + HOP_RANGE) % HOP_RANGE; //calculo el bit a actulizar

  if(value) dictionary->bitmaps[bitmap_to_update] |= (1U << (bit_to_update)); // prendo el bit del bitmap
  else dictionary->bitmaps[bitmap_to_update] &= ~(1U << (bit_to_update)); // apago el bit del bitmap
}
bool insert_node(dictionary_t *dictionary, size_t index, const char *key, void *value) 
{
  if(dictionary->nodes[index].key); // si ya hay un nodo en esa posicion, devuelvo false
  dictionary->nodes[index].key = strdup(key); 
  if(!dictionary->nodes[index].key)
  { 
    return false; //si no se pudo asignar memoria
  }
  dictionary->nodes[index].value = value;
  update_bitmap(dictionary, key, index, true); // prendo el bit del bitmap
  dictionary->size++;
  update_alpha(dictionary);
  return true;
}
size_t find_last_zero_position(unsigned int bitmap) {
    // invierto los bits
    unsigned int inverted = ~bitmap;
    // devuelvo el indice del primer bit en 1, como los invierte, es el primer 0 desde el LSB
    return (size_t) (__builtin_ctz(inverted));
}
__ssize_t get_key_index(dictionary_t *dictionary, const char *key) {
  size_t index = get_hash_index(dictionary, key);
  unsigned int bitmap = dictionary->bitmaps[index];

  while (bitmap != 0){
      // Encuentra el primer bit en 1 desde el más significativo y Calcula la posición del bit en el vecindario
      size_t bit_position = (sizeof(unsigned int) * 8 - 1) - __builtin_clz(bitmap);  
      if (strcmp(dictionary->nodes[index + bit_position].key, key) == 0) return index + bit_position;
      bitmap &= ~(1U << bit_position); // apaga el bit que ya revisamos 
  }
  return -1;
}
__ssize_t insert_in_neighborhood(dictionary_t * dictionary, const char * key, void * value, size_t neighbor_index){
  for (size_t i = 0; i < HOP_RANGE; i++) {
      size_t curr_index = (neighbor_index + i) % dictionary->capacity;
      if (!dictionary->nodes[curr_index].key) return i + neighbor_index; // si encontre un lugar vacio, devuelvo el indice
      // insert node, me actualiza el bitmap y el size   
  }
  return -1; // si no encontro lugar en el vecindario, false
}
bool is_at_edge(dictionary_t *dictionary,size_t index){
  size_t hash_index = get_hash_index(dictionary, dictionary->nodes[index].key);
  return ((hash_index - index + HOP_RANGE) % HOP_RANGE) == 31;
}
void swap_node_location(dictionary_t *dictionary,size_t current , size_t previous){
  Nodo temp = dictionary->nodes[current];
  dictionary->nodes[current] = dictionary->nodes[previous];
  dictionary->nodes[previous] = temp;

  update_bitmap(dictionary, dictionary->nodes[previous].key, previous, false); //apago el bit de donde lo muevo
  update_bitmap(dictionary, dictionary->nodes[previous].key, current, true);  //prendo el bit a donde lo mando
  //como el current es empty, no cambio su bitmap
}
bool move_keys_to_make_space(dictionary_t *dictionary, size_t index, size_t * empty_index){
  size_t i = index + HOP_RANGE; //arranco desde el final del vecindario 
  while (!dictionary->nodes[i % dictionary->capacity].key) i++;
  size_t gap = 0; //si no puedo intercambiar un nodo ,el nodo vacio esta en i

  while(i > index + HOP_RANGE){ // trato de traer el NULL hasta el vecindario + 1
      size_t previous = (i - 1 + dictionary->capacity) % dictionary->capacity;
      //si esta en el borde de su vecindario, no lo puedo mover para adelante por que lo saco

      if (!is_at_edge(dictionary, previous)) {
          swap_node_location(dictionary, i + gap , previous); 
          gap = 0;
      }

      else gap++;
      if(gap == HOP_RANGE - 1) return false; // si no puedo mover el NULL, no puedo hacer espacio
      i --;
  }

  size_t empty = (i + gap) % dictionary->capacity; //traje para atras lo mas posible al NULL, ahora tengo que insertarlo en el vecindario
  unsigned int neighborhood_bitmap = dictionary->bitmaps[index];

  while (neighborhood_bitmap != FULL_NEIGHBORHOOD){
      //voy a ir desde el LSB al MSB por las posiciones donde haya 0's
      size_t bit_position = find_last_zero_position(neighborhood_bitmap);
      size_t pos = bit_position + index;

      if (pos - get_hash_index(dictionary, dictionary->nodes[pos].key) + (empty - pos + index) < HOP_RANGE) {
          if(dictionary->nodes[index +bit_position].key)swap_node_location(dictionary, index + bit_position, empty);
          *empty_index = index + bit_position;
          return true;
      }
      neighborhood_bitmap |= (1U << bit_position); //predo el bit que ya revise 
  }
  return false; // no pude mover nodos de mi vecindario
}

dictionary_t *dictionary_create(destroy_f destroy) { 
  dictionary_t *dic = malloc(sizeof(dictionary_t));
  if(!dic) return NULL;

  dic->destroy = destroy;
  dic->size = 0;
  dic->nodes = calloc(INITIAL_CAPACITY, sizeof(Nodo));
  dic->bitmaps = calloc(INITIAL_CAPACITY, sizeof(unsigned int));
  if (!dic->nodes || !dic->bitmaps) {
      free(dic->nodes);
      free(dic->bitmaps);
      free(dic);
      return NULL;
  }

  dic->capacity = INITIAL_CAPACITY;
  dic->alpha = 0;
  return dic;
}

bool rehash(dictionary_t *dictionary) {
  size_t old_capacity = dictionary->capacity;
  Nodo *old_table = dictionary->nodes;
  unsigned int *old_bitmaps = dictionary->bitmaps;

  dictionary->capacity *= 2;
  dictionary->size = 0;
  dictionary->nodes = calloc(dictionary->capacity, sizeof(Nodo));
  dictionary->bitmaps = calloc(dictionary->capacity, sizeof(unsigned int));

  if (!dictionary->nodes || !dictionary->bitmaps) {
      free(dictionary->nodes); //bitmpas no lo deberia liberar porque no lo asigno
      dictionary->nodes = old_table;
      dictionary->bitmaps = old_bitmaps;
      dictionary->capacity = old_capacity;
      return false;
  }

  update_alpha(dictionary);

  for (size_t i = 0; i < old_capacity; i++) {
      if (old_table[i].key) {
          bool put = dictionary_put(dictionary, old_table[i].key, old_table[i].value);
          free(old_table[i].key);
          if (!put) { //si falla put libero memoria y devuelvo false
              free(old_table); 
              free(old_bitmaps);
              return false;
          }
      }
  }

  free(old_table); 
  free(old_bitmaps);
  return true;
}

bool dictionary_put(dictionary_t *dictionary, const char *key, void *value) {
  __ssize_t maybe_index = get_key_index(dictionary, key);
  if(maybe_index != -1) return replace_value(dictionary, maybe_index, value);

  size_t index = get_hash_index(dictionary, key);
  if (dictionary->alpha >= 0.55 || dictionary->bitmaps[index] == FULL_NEIGHBORHOOD) {
      if (!rehash(dictionary)) return false;
  }

  __ssize_t neighboor_index = insert_in_neighborhood(dictionary, key, value, index);
  if (neighboor_index != -1) return insert_node(dictionary, neighboor_index, key, value);
  
  size_t empty_index;
  if (!move_keys_to_make_space(dictionary, index, &empty_index)) {
      if (!rehash(dictionary)) return false;
      return dictionary_put(dictionary, key, value);
  }

  return insert_node(dictionary, empty_index, key, value);
}

void *dictionary_get(dictionary_t *dictionary, const char *key, bool *err) {
  *err = true;
  __ssize_t index = get_key_index(dictionary, key);
  if(index == -1) return NULL;
  *err = false;
  return dictionary->nodes[index].value;
}

bool dictionary_delete(dictionary_t *dictionary, const char *key) {
  __ssize_t index = get_key_index(dictionary, key);
  if(index == -1) return false; // si no existe la clave, no se puede borrar
  if(dictionary->destroy) dictionary->destroy(dictionary->nodes[index].value); // si existe destroy, la uso
  update_bitmap(dictionary, key, index,false); // apago el bit del bitmap
  free(dictionary->nodes[index].key); 
  dictionary->nodes[index].value = NULL;
  dictionary->nodes[index].key = NULL;
  dictionary->size--;
  update_alpha(dictionary);
  return true;
}

void *dictionary_pop(dictionary_t *dictionary, const char *key, bool *err) {
  *err = true;
  __ssize_t index = get_key_index(dictionary,key);
  if(index == -1) return NULL; 

  *err = false;
  void * valor = dictionary->nodes[index].value;
  update_bitmap(dictionary, key, (size_t) (index), false); // apago el bit del bitmap
  free(dictionary->nodes[index].key);

  dictionary->nodes[index].value = NULL;
  dictionary->nodes[index].key = NULL;
  if (dictionary->size > 0) dictionary->size--;
  update_alpha(dictionary);

  return valor;
}

bool dictionary_contains(dictionary_t *dictionary, const char *key) {
  return get_key_index(dictionary, key) != -1;}

size_t dictionary_size(dictionary_t *dictionary) { 
  return dictionary->size; }

void dictionary_destroy(dictionary_t *dictionary){
  for(int i = 0; i < dictionary->capacity; i++){
      if(dictionary->nodes[i].key) {
          if(dictionary->destroy) dictionary->destroy(dictionary->nodes[i].value); // si existe destroy, la uso
          free(dictionary->nodes[i].key);
      }
  }
  
  free(dictionary->bitmaps);
  free(dictionary->nodes);
  free(dictionary);
}


