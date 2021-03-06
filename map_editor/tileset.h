#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct TerrainType {
  Texture tex;
  char* tex_path;
  int id;
} TerrainType;

typedef struct TypeSet {
  Vector types;
  char* name;
} TypeSet;

TypeSet NewTypeSet(char* name) {
  TypeSet output;
  output.types = NewVector(sizeof(TerrainType));
  output.name = calloc(max_filename_length, sizeof(char));
  strcpy(output.name, name);
  return output;
}

void NewTypeToTileSet(TypeSet* typeset, TerrainType template) {
  TerrainType t = { LoadTexture(template.tex_path), template.tex_path, typeset->types.length };
  t.tex_path = calloc(64, sizeof(char));
  strcpy(t.tex_path, template.tex_path);
  AddToVector(&(typeset->types), &t);
}

void ExportTypeSet(TypeSet typeset, char* file_name) {
  FILE* target = fopen(file_name, "w");
  for(int i = 0; i < typeset.types.length; i++) {
    fprintf(target, "%i %s \n",((TerrainType*)AccessVectorElement(i, typeset.types))->id, ((TerrainType*)AccessVectorElement(i, typeset.types))->tex_path);
  }
  fclose(target);
}

TypeSet LoadTypeSet(char* target_file) {
  FILE* f = fopen(target_file, "r");
  if (!f) {
    perror (target_file);
    exit (1);
  }
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  rewind(f);
  char *string = malloc(fsize + 1);
  fread(string, fsize, 1, f);
  fclose(f);
  
  int j = 0;
  int nth_element = 0;
  int i = 0;
  int r = 0;
  char* eval = calloc(fsize, sizeof(char));

  TypeSet output = NewTypeSet(target_file);
  TerrainType type_being_worked_on;
  type_being_worked_on.tex_path = calloc(fsize, sizeof(char));
  
  for(i = 0; i < fsize; i++) {
    if(string[i] != '\n' && string[i] != ' ' ) {
      eval[r] = string[i];
      r++;
    }
    else {
      switch(j) {
      case 0: type_being_worked_on.id = atoi(eval); break;
      case 1:
	type_being_worked_on.tex = LoadTexture(eval);
	strncpy(type_being_worked_on.tex_path, eval, strlen(eval));
	break;
      }
      free(eval);
      eval = calloc(fsize, sizeof(char));
      j++;
      r = 0;
      if(string[i] == '\n') {
	AddToVector(&output.types, &type_being_worked_on);
	type_being_worked_on.tex_path = calloc(fsize, sizeof(char));
	j = 0;
	nth_element++;
      }
    }
  }
  free(type_being_worked_on.tex_path);
  free(eval);
  free(string);
  return output;
}

typedef struct Tile {
  TerrainType** type;
  int2 cl;
  Vector2 pos;
  bool border;
} Tile;

const int n_tile_elements = 3;

typedef struct TileSet {
  Vector tiles;
  int2 size;
  TypeSet* typeset;
  Texture* chunks;
} TileSet;

const int tilesize = 16;

TileSet NewTileSet( TypeSet t, TypeSet t1, TypeSet t2, int2 size ) {
  Vector tiles = NewVector(sizeof(Tile));
  TypeSet* typeset = malloc(3 * sizeof(TypeSet));
  typeset[0] = t;
  typeset[1] = t1;
  typeset[2] = t2;

  for(int l = 0; l < size.b; l++ ) {
    for(int c = 0; c < size.a; c++ ) {
      TerrainType** tile_types = malloc(3 * sizeof(TerrainType*));
      for(int i = 0; i < 3; i++) {
	tile_types[i] = AccessVectorElement(0, typeset[0].types );
      }
      Tile tile = {tile_types, (int2){c, l}, (Vector2){c*tilesize, l*tilesize}, false};
      if(c == size.a - 1 || c == 0 || l == size.b - 1 || l == 0 ) tile.border = true;
      AddToVector(&tiles, &tile);
    }
  }
  return (TileSet){ tiles, size, typeset };
}

TileSet NewDefaultTileSet(TerrainType default_type, int2 size) {
  TypeSet t[3];
  t[0] = NewTypeSet("background");
  t[1] = NewTypeSet("overlay1");
  t[2] = NewTypeSet("overlay2");
  for(int i = 0; i < 3; i++) {
    NewTypeToTileSet(&t[i], default_type);
  }
  return (NewTileSet(t[0], t[1], t[2], size));
}

TileSet MergeTileSet(TileSet dest, const TileSet src) {
  int2 size = { 0 };
  if(src.size.b < dest.size.b) size.b = src.size.b - 1;
  else size.b = dest.size.b -1;
  if(src.size.a < dest.size.a) size.a = src.size.a - 1;
  else size.a = dest.size.a;
  for(int l = 0; l < size.b; l++ ) {
    for(int c = 0; c < size.a; c++ ) {
      SetVectorValue(&dest.tiles, c + l * dest.size.a, AccessVectorElement( c + l * src.size.a, src.tiles));      
      if(c == dest.size.a - 1 || c == 0 || l == dest.size.b - 1 || l == 0 ) ((Tile*)AccessVectorElement( c + l*dest.size.a, dest.tiles))->border = true;
      else ((Tile*)AccessVectorElement( c + l * dest.size.a, dest.tiles))->border = false;
    }
  }
  return dest;
}

void ExportTileSet(TileSet t, char* target_name) {
  FILE* target = fopen( target_name, "w");
  fprintf(target, "%s; %i; %i; \n", t.typeset[0].name , t.size.a, t.size.b);
  for(int i = 0; i < t.size.a*t.size.b; i++) {
    fprintf(target, "%i %i \n", i,
	   ((Tile*)AccessVectorElement(i, t.tiles))->type[0]->id);

  }
  fclose(target);
}

void ExportMap(TileSet t, char* target_dir) {
  struct stat st = {0};
  if (stat(target_dir, &st) == -1) {
    mkdir(target_dir, S_IRWXU);  
  }
  char* cpy = calloc(1400, sizeof(char));
  strcpy( cpy, target_dir);
  ExportTypeSet(t.typeset[0], strcat(cpy, "/background"));
  strcpy( cpy, target_dir);
  ExportTileSet(t, strcat(cpy, "/map"));
}

TileSet LoadTileSet(const char* target_file) {
  char* filen_cpy = malloc(max_filename_length * sizeof(char));
  strcpy(filen_cpy, target_file);
  strcat(filen_cpy, "/");
  FILE* f = fopen(strcat(filen_cpy, "map\0"), "r");
  strcpy(filen_cpy, target_file);
  strcat(filen_cpy, "/");
  if (!f) {
    perror (target_file);
    exit (1);
  }
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  rewind(f);
  char *string = malloc(fsize + 1);
  fread(string, fsize, 1, f);
  fclose(f);
  
  int j = 0;
  int i = 0;
  int r = 0;
  char* eval = calloc(fsize, sizeof(char));
  char* typeset_name = calloc(fsize, sizeof(char)); 
  int size[2] = {0};
  for(i = 0; i < fsize; i++) {
    if(string[i] != ';' && string[i] != ' ' ) {
      eval[r] = string[i];
      r++;
    }
    else if(string[i] == ';') {
      if(j == 0) strncpy(typeset_name, eval, strlen(eval)*sizeof(char));
      else size[j - 1] = atoi(eval);
      j++;
      r = 0;
      free(eval);
      eval = calloc(fsize, sizeof(char));
    }
    if(string[i] == '\n') {
      break;
    }
  }
  free(eval);
  eval = calloc(fsize, sizeof(char));
  TileSet output = NewTileSet(LoadTypeSet(strcat(filen_cpy, typeset_name)), LoadTypeSet(strcat(filen_cpy, typeset_name)), LoadTypeSet(strcat(filen_cpy, typeset_name)), (int2){size[0], size[1]});
  free(filen_cpy);
  free(typeset_name);
  j = 0;
  int nth_element = 0;
  int nth_tile = 0;
  for(int k = i + 1; k < fsize; k++) {
    if(string[k] !=  '\n' && string[k] != ' ') {
      eval[r] = string[k];
      r++;
    }
    else {
      switch (nth_element)
	{
	case 0:nth_tile = atoi(eval); break;
	case 1: ((Tile*)AccessVectorElement(nth_tile, output.tiles))->type[0] =
	    ((TerrainType*)AccessVectorElement(atoi(eval), output.typeset[0].types)); break;
	}
      nth_element++;
      free(eval);
      r = 0;
      eval = calloc(fsize, sizeof(char));
      if(string[k] == '\n') {
	nth_element = 0;
      }
    }
  }
  free(eval);
  free(string);
  return output;
}

const int2 chunk_size = { 16, 16 };

Texture* GenerateChunks(TileSet tileset) {
  Vector output = NewVector(sizeof(Texture));
  RenderTexture tex = LoadRenderTexture((chunk_size.a + 1) * tilesize, (chunk_size.b +1) * tilesize);
  for(int l = 0; l < tileset.size.a/chunk_size.a; l++) {
    for(int c = 0; c < tileset.size.b/chunk_size.b; c++) {
      BeginTextureMode(tex);
      ClearBackground(BLANK);
      for(int k = (c)* chunk_size.b; k < (c + 1) * chunk_size.b; k++) {
	for(int j = (l)* chunk_size.a; j < (l + 1 ) * chunk_size.a; j++) {
	  if(!((Tile*)AccessVectorElement( j + k * tileset.size.a, tileset.tiles ))->border) {
	    DrawTextureRec(((Tile*)AccessVectorElement( j + k * tileset.size.a, tileset.tiles ))->type[0]->tex,
			   (Rectangle){ 0, 0, ((Tile*)AccessVectorElement( j + k * tileset.size.a, tileset.tiles ))->type[0]->tex.width, -((Tile*)AccessVectorElement( j + k * tileset.size.a, tileset.tiles ))->type[0]->tex.height},
			   (Vector2){ ((Tile*)AccessVectorElement( j + k * tileset.size.a, tileset.tiles))->pos.x, tileset.size.b * tilesize - ((Tile*)AccessVectorElement( j + k * tileset.size.a, tileset.tiles))->pos.y }, WHITE);
	  }
	}
      }
      EndTextureMode();
      AddToVector(&output, &(tex.texture));
    }
  }
  return ((Texture*)output.data);
}

void RenderTileSet(TileSet tileset, int2 target) {
  DrawTextureRec(tileset.chunks[0], (Rectangle){ 0, 0, chunk_size.a*tilesize, (chunk_size.b*tilesize) }, (Vector2){ 0, 0 }, WHITE);
}

void RenderTileSetRaw(TileSet t, int2 target) {
  for(int i = 0; i < t.size.a * t.size.b; i++) {
    if(!((Tile*)AccessVectorElement( i, t.tiles ))->border) {
      for(int j = 0; j < 3; j++) {
	DrawTextureEx(((Tile*)AccessVectorElement( i, t.tiles ))->type[j]->tex,
		    (Vector2){ ((Tile*)AccessVectorElement( i, t.tiles))->pos.x, ((Tile*)AccessVectorElement( i, t.tiles))->pos.y + tilesize - ((Tile*)AccessVectorElement( i, t.tiles ))->type[j]->tex.height } ,
		  0.0f, 1.0f, WHITE);
      }
    }
  }
}
