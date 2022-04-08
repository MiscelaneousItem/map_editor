#include <stdlib.h>
#include <stdio.h>
#include <string.h>

///Vectors & Vector Related Functions

typedef struct int2 {
  int a, b;
} int2;

typedef struct Vector {
  int length;
  int capacity;
  int size_of_element;
  char* data;
} Vector;

const Vector NewVector(int target_size_of_element) {
  Vector output = {0};
  output.size_of_element = target_size_of_element;
  //output.data = malloc(target_size_of_element);
  return output;
}

void* AccessVectorElement(int index, Vector components) {
  return components.data + index * components.size_of_element;
}

int RemoveFromVector(Vector* vector, void* val) {
  int i;
  for(i = 0; i < vector->length; i++) {
    if(memcmp(AccessVectorElement(i, *vector), val, vector->size_of_element) == 0) break;
    if(i == vector->length) return i;
  }
  Vector cpy = *vector;
  for(int j = i; j < vector->length; j++) { 
  memcpy(AccessVectorElement(j, *vector),
  	 AccessVectorElement(j+1, cpy),
	 vector->size_of_element);
  }
  vector->length--;
  return i;
}

void SetVectorValue(Vector* vector, int index, void* val) {
  if(vector->length == vector->capacity) {
    vector->capacity+=10;
    vector->data = realloc(vector->data, vector->capacity * vector->size_of_element);
  }
  memcpy(AccessVectorElement(index, *vector), val, vector->size_of_element);
}

int AddToVector(Vector* vector, void* val ) {
  SetVectorValue(vector, vector->length, val);
  int output = vector->length;
  vector->length++;
  return output;
}

void RemoveFromVectorInt(Vector* vector, int val) {
  Vector cpy = (*vector);
  (*vector) = NewVector(vector->size_of_element);
  for(int j = 0; j < cpy.length; j++) {
    if(j != val) {
      AddToVector(vector, AccessVectorElement(j, cpy));
    }
  }
}
