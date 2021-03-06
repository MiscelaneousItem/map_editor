#include "raylib.h"
#include <stdlib.h>
#include <math.h>

const int max_filename_length = 64;
char eof = '\0';

#include "vector.h"
#include "tileset.h"

bool RectangleClicked(Rectangle rec) {
  return (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rec));
}

void InputToChar(Vector* storage) {
  int key = GetCharPressed();
  while(key > 0) {
    if((key >= 32) && (key <= 125)) {
      SetVectorValue(storage, storage->length, &key);
      storage->length++;
      SetVectorValue(storage, storage->length, &eof);
    }
    key = GetCharPressed();
  }
  if(IsKeyPressed(KEY_BACKSPACE)) {
    storage->length--;
    if (storage->length < 0) storage->length = 0;
    SetVectorValue(storage, storage->length, &eof);
  }
};

void DrawTextComposite(const char* str1, const char* str2, Color c1, Color c2, int fontsize,Vector2 pos) {
  DrawText(str1, pos.x, pos.y , fontsize, c1);
  DrawText(str2, pos.x + MeasureText(str1, fontsize) + MeasureText(" ", fontsize)*2, pos.y, fontsize, c2);
}

void DrawTextComposite3(const char* str1, const char* str2, const char* str3, Color c1, Color c2, Color c3, int fontsize,Vector2 pos) {
  DrawText(str1, pos.x, pos.y , fontsize, c1);
  DrawText(str2, pos.x + MeasureText(str1, fontsize) + MeasureText(" ", fontsize)*2, pos.y , fontsize, c2);
  DrawText(str3, pos.x + MeasureText(str1, fontsize) + MeasureText(str2, fontsize) + MeasureText(" ", fontsize)*4, pos.y , fontsize, c3);
}

bool ButtonCompositeClicked(const char* str1, const char* str2, int fontsize,Vector2 pos) {
  return RectangleClicked((Rectangle){pos.x, pos.y, pos.x + fontsize + MeasureText(TextFormat("%s  %s", str1, str2), fontsize), pos.y + fontsize});
}

void GuiUpdateTypeEdit(bool* clicks, TerrainType t, int fontsize, Vector2 pos) {
  if(ButtonCompositeClicked("texture path:", TextFormat("%s", t.tex_path), fontsize, (Vector2){pos.x + fontsize, pos.y + fontsize})) {
    clicks[0] = true;
    clicks[1] = false;
  }
  else if(ButtonCompositeClicked("id:", TextFormat("%i", t.id), fontsize, (Vector2){pos.x + fontsize, pos.y + fontsize*2})) {
    clicks[1] = true;
    clicks[0] = false;
  }
  else if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    clicks[0] = false;
    clicks[1] = false;
  }
}

void GuiRenderTypeEdit(TerrainType* t, Vector2 pos, bool* clicks, Vector* input, char** current_error_msg) {
  const int fontsize = 18;
  GuiUpdateTypeEdit(clicks, *t, fontsize, pos);
  DrawRectangleV(pos, (Vector2){GetScreenWidth(),GetScreenHeight()}, DARKGRAY);
  if(!clicks[0]) {
    DrawTextComposite("texture path:", TextFormat("%s", t->tex_path), GRAY, WHITE, fontsize, (Vector2){pos.x + fontsize, pos.y + fontsize});
  }
  else {
    DrawTextComposite3("texture path:", TextFormat("%s", input->data), *(current_error_msg), GRAY, WHITE, RED, fontsize, (Vector2){pos.x + fontsize, pos.y + fontsize});
    InputToChar(input);
    if(IsKeyPressed(KEY_ENTER)) {
      FILE* f = fopen(input->data, "r");
      if( f > 0) {
	strcpy(t->tex_path, input->data);
	t->tex = LoadTexture(input->data);
	SetVectorValue(input, 0, &eof);
	input->length = 0;
	*current_error_msg = "\0";
	clicks[0] = false;
	fclose(f);      
      }
      else *(current_error_msg) = "[no such file or directory]";
    }
  }
  DrawTextComposite("id:", TextFormat("%i", t->id), GRAY, WHITE, fontsize, (Vector2){pos.x + fontsize, pos.y + fontsize*2});
}

void GuiUpdateTypeSelect(int* selected_tile, int* selected_type, int current_layer, TileSet* t, Vector2 pos) {
  (*selected_type) -= GetMouseWheelMove();
  if((*selected_type) < 0) (*selected_type) = t->typeset[current_layer].types.length;
  else if ((*selected_type) > t->typeset[current_layer].types.length) (*selected_type) = 0;
    
  if(CheckCollisionPointRec(GetMousePosition(), (Rectangle){0, 0, (t->size.a - 1) * tilesize, (t->size.b - 1) * tilesize}))
    (*selected_tile) = trunc(GetMousePosition().x/tilesize) +
      trunc(GetMousePosition().y/tilesize) * t->size.a;
  else (*selected_tile) = -1;

  for(int i = 0; i < t->typeset[current_layer].types.length + 1; i++) {
    if(CheckCollisionPointRec(GetMousePosition(), (Rectangle){ pos.x - 0.5 * tilesize, pos.y + (tilesize*2.75)*i,  3 * tilesize, 3 * tilesize }) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      (*selected_type) = i;
    }
  }

  if(IsKeyPressed(KEY_DELETE) && (*selected_type) != t->typeset[current_layer].types.length && (*selected_type) != 0 ) {
    RemoveFromVectorInt((&t->typeset[current_layer].types), *selected_type);
    for(int i = 0; i < t->tiles.length; i++) {
      if(((Tile*)AccessVectorElement(i, t->tiles))->type[current_layer]->id == (*selected_type))((Tile*)AccessVectorElement(i, t->tiles))->type[current_layer] = ((TerrainType*)AccessVectorElement(0, t->typeset[current_layer].types));
    }
    (*selected_type)--;
  }
}

#define AGRAY CLITERAL (Color){127, 127, 127, 75}
#define ADARKGRAY CLITERAL (Color){100, 100, 100, 255}

void GuiRenderTypeSelect(int selected_type, TypeSet t, Vector2 pos, Texture2D plus_tex){
  DrawRectangleV(pos, (Vector2){tilesize*2.5, GetScreenHeight()}, AGRAY);
  DrawRectangleV( (Vector2){ pos.x - 0.5 * tilesize, pos.y + (tilesize*2.75)*selected_type}, (Vector2){ 3 * tilesize, 3 * tilesize }, DARKGRAY);
  for(int i = 0; i <= t.types.length; i++) {
    float offset = 0.;
    float scale = 2.;
    if(i == selected_type) {
      offset = 2/2.5 * tilesize;
      scale = 2.5;
    }
    if(i < t.types.length) DrawTextureEx( ((TerrainType*)AccessVectorElement(i, t.types))->tex, (Vector2){pos.x + 0.25 * tilesize - offset/1.5, pos.y + tilesize*2.75*i + tilesize*0.5 - offset/3. }, 0.f, scale, WHITE);
    else DrawTextureEx(plus_tex, (Vector2){pos.x + 0.25 * tilesize - offset/1.5, pos.y + tilesize*2.75*t.types.length + tilesize*0.5 - offset/3. }, 0.f, scale, WHITE);
  }
}

void hobbes_logger(int msgType, const char* text, va_list args){
  switch (msgType) {
  case LOG_ERROR: printf("[ERROR]: %s \n", text); break;
  case LOG_WARNING: printf("[WARNNG]: %s \n", text); break;
  case LOG_DEBUG: printf("[DEBUG]: %s \n", text); break;
  }
}

int main() {

  SetTraceLogCallback(hobbes_logger);

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  
  InitWindow(890, 600, "project");
  SetExitKey(KEY_NULL);

  TerrainType default_type = { LoadTexture("null.png"), "", 0};
  default_type.tex_path = calloc(max_filename_length, sizeof(char));
  strcpy(default_type.tex_path, "null.png");

  int selected_tile = -1;
  int selected_type = 0;
  int current_layer = 0;
  TileSet current_tileset = NewDefaultTileSet(default_type, (int2){16, 16});
  current_tileset.chunks = GenerateChunks(current_tileset);

  int current_dialog_option = -1;
  char* current_dialog_option_name = "\0";

  char* current_error_msg = "\0";
  
  bool type_edit_clicks[2] = { 0 };
  
  Vector input = NewVector(sizeof(char));
  SetVectorValue(&input, 0,  &eof);

  Texture plus_tex = LoadTexture("plus.png");
  
  while(!WindowShouldClose()) {

    if(CheckCollisionPointRec(GetMousePosition(), (Rectangle){ GetScreenWidth()/1.5  - 2.5*tilesize - 0.5 * tilesize, 0 + (tilesize*2.75)*current_tileset.typeset[current_layer].types.length,  3 * tilesize, 3 * tilesize }) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && selected_type == current_tileset.typeset[current_layer].types.length) {
      NewTypeToTileSet(&(current_tileset.typeset[current_layer]), default_type);
    }

    GuiUpdateTypeSelect(&selected_tile, &selected_type, current_layer, &current_tileset, (Vector2){ GetScreenWidth()/1.5 - 2.5*tilesize, 0 });

    if(current_dialog_option != -1) {
      if(current_dialog_option == 0) {
	current_dialog_option_name = "[export file to]: ";
	if(IsKeyPressed(KEY_ENTER)) {
	  ExportMap(current_tileset, input.data);
	  SetVectorValue(&input, 0, &eof);
	  input.length = 0;
	  current_dialog_option = -1;
	  current_dialog_option_name = "\0";
	}
      }
      else if(current_dialog_option == 1) {
	current_dialog_option_name = "[load file]:";
	    if(IsKeyPressed(KEY_ENTER)) {
	      FILE* f = fopen(input.data, "r");
	      if(f > 0) {
		
		for(int i = 0; i < current_tileset.size.a * current_tileset.size.b; i++) {
		  free(((Tile*)AccessVectorElement(i, current_tileset.tiles))->type);
		}
		free(current_tileset.tiles.data);
  
		for(int i = 0; i < 3; i++) {
		  for(int j = 0; j < current_tileset.typeset[i].types.length; j++) {
		    free(((TerrainType*)AccessVectorElement(j, current_tileset.typeset[i].types))->tex_path);
		  };
		  free(current_tileset.typeset[i].name);
		  free(current_tileset.typeset[i].types.data);
		};
		free(current_tileset.typeset);
		free(current_tileset.chunks);

		current_tileset = LoadTileSet(input.data);
		SetVectorValue(&input, 0, &eof);
		input.length = 0;
		current_dialog_option = -1;
		current_dialog_option_name = "\0";
		current_error_msg = "\0";
		fclose(f);
	      }
	      else current_error_msg = "[no such file or directory]";
	    }
      }
      else if(current_dialog_option == 2) {
	current_dialog_option_name = "[resize current tileset to]:";
	    if(IsKeyPressed(KEY_ENTER)) {
	      TileSet t = NewTileSet(current_tileset.typeset[0], current_tileset.typeset[1], current_tileset.typeset[2], (int2){atoi(input.data), atoi(input.data)});
	      current_tileset = MergeTileSet(t, current_tileset);
	      free(current_tileset.chunks);
	      current_tileset.chunks = GenerateChunks(current_tileset);
	      SetVectorValue(&input, 0, &eof);
	      input.length = 0;
	      current_dialog_option = -1;
	      current_dialog_option_name = "\0";
	    }
      }
      InputToChar(&input);
    }
    
    if(IsKeyDown(KEY_F1)) current_dialog_option = 0;
    if(IsKeyDown(KEY_F2)) current_dialog_option = 1;
    if(IsKeyDown(KEY_F3)) current_dialog_option = 2;
    if(IsKeyDown(KEY_ESCAPE)) current_dialog_option = -1;

    if(IsKeyPressed(KEY_ONE)) current_layer = 0;
    if(IsKeyPressed(KEY_TWO)) current_layer = 1;
    if(IsKeyPressed(KEY_THREE)) current_layer = 2;

    if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && selected_tile != -1 && selected_type != current_tileset.typeset[0].types.length) {
	((Tile*)AccessVectorElement(selected_tile, current_tileset.tiles))->type[current_layer] = ((TerrainType*)AccessVectorElement(selected_type, current_tileset.typeset[current_layer].types));
    }
    
    BeginDrawing();
    
      ClearBackground(BLACK);
      RenderTileSetRaw(current_tileset, (int2){0, 0});
      
      if(selected_type < 0) selected_type = current_tileset.typeset[current_layer].types.length;
      else if (selected_type > current_tileset.typeset[current_layer].types.length) selected_type = 0;
      
      if(selected_type != current_tileset.typeset[current_layer].types.length) {
	GuiRenderTypeEdit(((TerrainType*)AccessVectorElement(selected_type, current_tileset.typeset[current_layer].types)), (Vector2){GetScreenWidth()/1.5, 0}, type_edit_clicks, &input, &current_error_msg);
      }
      else {
	GuiRenderTypeEdit(&default_type, (Vector2){GetScreenWidth()/1.5, 0}, type_edit_clicks, &input, &current_error_msg);
      }
      GuiRenderTypeSelect(selected_type, current_tileset.typeset[current_layer], (Vector2){ GetScreenWidth()/1.5 - 2.5*tilesize, 0 }, plus_tex);
      if(current_dialog_option != -1) {
	DrawText(TextFormat("%s %s", current_dialog_option_name, input.data), 2, 2 , 16.,  WHITE);
      };
      DrawText(TextFormat("%s", current_error_msg), MeasureText(TextFormat("%s %s", current_dialog_option_name, input.data), 16) + MeasureText(" ", 16)*2, 2 , 16.,  RED);

    EndDrawing();
    
  }
  
  CloseWindow();
  
  free(default_type.tex_path);
  
  for(int i = 0; i < current_tileset.size.a * current_tileset.size.b; i++) {
      free(((Tile*)AccessVectorElement(i, current_tileset.tiles))->type);
  }
  free(current_tileset.tiles.data);
  
  for(int i = 0; i < 3; i++) {
    for(int j = 0; j < current_tileset.typeset[i].types.length; j++) {
      free(((TerrainType*)AccessVectorElement(j, current_tileset.typeset[i].types))->tex_path);
    };
    free(current_tileset.typeset[i].name);
    free(current_tileset.typeset[i].types.data);
  };
  free(current_tileset.typeset);
  free(current_tileset.chunks);
  free(input.data);

  return 0;
};
