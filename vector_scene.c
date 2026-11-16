// AED 2026/2027 - TAD VectorScene
// Nome: (preencher)    Nº: (preencher)
// Nome: (preencher)    Nº: (preencher)

#include "vector_scene.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "primitive_internal.h"

// a cena guarda um array dinâmico de ponteiros e é dona das primitivas
struct _VectorScene {
  int width;                int height;               Color bg_color;           Primitive** primitives;   size_t count;             size_t capacity;          int next_auto_id;       };

// capacidade inicial do array de primitivas
#define INITIAL_SCENE_CAPACITY 8

// grava o buffer RGB num ficheiro PPM (fornecido)
static bool save_ppm_image(const char* filename, int width, int height,
                           const uint8_t* pixel_buffer) {
  if (!filename || !pixel_buffer || width <= 0 || height <= 0) {
    return false;
  }

  FILE* file = fopen(filename, "wb");
  if (!file) {
    return false;
  }

  fprintf(file, "P6\n%d %d\n255\n", width, height);

  size_t total_bytes = (size_t)width * height * 3;
  size_t written = fwrite(pixel_buffer, 1, total_bytes, file);
  fclose(file);

  return written == total_bytes;
}

int vector_scene_get_width(const VectorScene* scene) {
  if (!scene) {
    return 0;
  }
  return scene->width;
}

int vector_scene_get_height(const VectorScene* scene) {
  if (!scene) {
    return 0;
  }
  return scene->height;
}

Color vector_scene_get_background_color(const VectorScene* scene) {
  if (!scene) {
    Color black = {0, 0, 0};
    return black;
  }
  return scene->bg_color;
}

size_t vector_scene_get_count(const VectorScene* scene) {
  if (!scene) {
    return 0;
  }
  return scene->count;
}

Primitive* vector_scene_get_primitive_by_index(const VectorScene* scene,
                                               size_t index) {
  if (!scene || index >= scene->count) {
    return NULL;
  }
  return scene->primitives[index];
}

Primitive* vector_scene_get_primitive_by_id(const VectorScene* scene, int id) {
  if (!scene || id < 0) {
    return NULL;
  }

  for (size_t i = 0; i < scene->count; i++) {
    if (scene->primitives[i] && scene->primitives[i]->id == id) {
      return scene->primitives[i];
    }
  }
  return NULL;
}

VectorScene* vector_scene_create(int width, int height, Color bg_color) {
  if (width <= 0 || height <= 0) {
    return NULL;
  }

  VectorScene* scene = malloc(sizeof(VectorScene));
  if (scene == NULL) {
    return NULL;
  }

  scene->primitives = malloc(INITIAL_SCENE_CAPACITY * sizeof(Primitive*));
  if (scene->primitives == NULL) {
    free(scene);
    return NULL;
  }

  scene->width = width;
  scene->height = height;
  scene->bg_color = bg_color;
  scene->count = 0;
  scene->capacity = INITIAL_SCENE_CAPACITY;
  scene->next_auto_id = 0;
  return scene;
}

// destrói todas as primitivas e depois a cena
void vector_scene_destroy(VectorScene* scene) {
  if (!scene) {
    return;
  }

  for (size_t i = 0; i < scene->count; i++) {
    primitive_destroy(scene->primitives[i]);
  }
  free(scene->primitives);
  free(scene);
}

int vector_scene_add_primitive(VectorScene* scene, Primitive* p) {
  if (!scene || !p) {
    return -1;
  }

  // array cheio: duplica a capacidade
  if (scene->count == scene->capacity) {
    size_t new_capacity = scene->capacity * 2;
    Primitive** new_array =
        realloc(scene->primitives, new_capacity * sizeof(Primitive*));
    if (new_array == NULL) {
      return -1;
    }
    scene->primitives = new_array;
    scene->capacity = new_capacity;
  }

  // ID automático; os IDs explícitos nunca são repetidos pelo gerador
  if (p->id < 0) {
    p->id = scene->next_auto_id++;
  } else if (p->id >= scene->next_auto_id) {
    scene->next_auto_id = p->id + 1;
  }

  scene->primitives[scene->count] = p;
  scene->count++;
  primitive_update_aabb(p);
  return p->id;
}

// destrói a primitiva e desloca as seguintes para a esquerda
bool vector_scene_remove_primitive(VectorScene* scene, int id) {
  if (!scene || id < 0) {
    return false;
  }

  for (size_t i = 0; i < scene->count; i++) {
    if (scene->primitives[i]->id == id) {
      primitive_destroy(scene->primitives[i]);
      for (size_t j = i; j + 1 < scene->count; j++) {
        scene->primitives[j] = scene->primitives[j + 1];
      }
      scene->count--;
      return true;
    }
  }
  return false;
}

bool vector_scene_translate_primitive(VectorScene* scene, int id, double dx,
                                      double dy) {
  if (!scene) {
    return false;
  }

  Primitive* p = vector_scene_get_primitive_by_id(scene, id);
  if (p == NULL) {
    return false;
  }
  primitive_translate(p, dx, dy);
  return true;
}

bool vector_scene_set_z_index(VectorScene* scene, int id, int new_z_index) {
  if (!scene) {
    return false;
  }

  Primitive* p = vector_scene_get_primitive_by_id(scene, id);
  if (p == NULL) {
    return false;
  }
  p->z_index = new_z_index;
  return true;
}

// Insertion Sort estável: conta comparações e deslocamentos
static void insertion_sort_by_z(Primitive** arr, size_t count,
                                SortStats* stats) {
  if (count < 2) {
    return;
  }

  for (size_t i = 1; i < count; i++) {
    Primitive* key = arr[i];
    size_t j = i;

    while (j > 0) {
      if (stats) stats->comparisons++;
      if (arr[j - 1]->z_index > key->z_index) {
        arr[j] = arr[j - 1];
        if (stats) stats->swaps++;
        j--;
      } else {
        break;
      }
    }
    arr[j] = key;
  }
}

// junta [left..mid] com [mid+1..right]: conta comparações e cópias
static void merge_by_z(Primitive** arr, Primitive** temp_arr, size_t left,
                       size_t mid, size_t right, SortStats* stats) {
  size_t i = left;
  size_t j = mid + 1;
  size_t k = left;

  while (i <= mid && j <= right) {
    if (stats) stats->comparisons++;
    // <= mantém a ordem dos empates (estável)
    if (arr[i]->z_index <= arr[j]->z_index) {
      temp_arr[k++] = arr[i++];
    } else {
      temp_arr[k++] = arr[j++];
    }
    if (stats) stats->swaps++;
  }
  while (i <= mid) {
    temp_arr[k++] = arr[i++];
    if (stats) stats->swaps++;
  }
  while (j <= right) {
    temp_arr[k++] = arr[j++];
    if (stats) stats->swaps++;
  }

  for (k = left; k <= right; k++) {
    arr[k] = temp_arr[k];
    if (stats) stats->swaps++;
  }
}

// Merge Sort recursivo
static void merge_sort_by_z(Primitive** arr, Primitive** temp_arr, size_t left,
                            size_t right, SortStats* stats) {
  if (left >= right) {
    return;
  }

  size_t mid = left + (right - left) / 2;
  merge_sort_by_z(arr, temp_arr, left, mid, stats);
  merge_sort_by_z(arr, temp_arr, mid + 1, right, stats);
  merge_by_z(arr, temp_arr, left, mid, right, stats);
}

// aloca o array temporário e ordena (false se faltar memória)
static bool merge_sort_array(Primitive** arr, size_t count, SortStats* stats) {
  if (count < 2) {
    return true;
  }
  Primitive** temp_arr = malloc(count * sizeof(Primitive*));
  if (temp_arr == NULL) {
    return false;
  }
  merge_sort_by_z(arr, temp_arr, 0, count - 1, stats);
  free(temp_arr);
  return true;
}

// ordena por z_index crescente; sem memória para o Merge Sort usa Insertion Sort
void vector_scene_sort_by_z(VectorScene* scene, bool use_fast_sort,
                            SortStats* stats) {
  if (scene == NULL) return;

  if (stats) {
    stats->comparisons = 0;
    stats->swaps = 0;
  }
  if (scene->count < 2) return;

  if (use_fast_sort) {
    if (!merge_sort_array(scene->primitives, scene->count, stats)) {
      if (stats) {
        stats->comparisons = 0;
        stats->swaps = 0;
      }
      insertion_sort_by_z(scene->primitives, scene->count, stats);
    }
  } else {
    insertion_sort_by_z(scene->primitives, scene->count, stats);
  }
}

// devolve a primitiva visível no ponto; com is_opt testa primeiro a AABB
Primitive* vector_scene_pick_top_primitive(const VectorScene* scene, Point pt,
                                           bool is_opt, PickStats* stats) {
  if (!scene) {
    return NULL;
  }

  if (stats) {
    stats->aabb_tests = 0;
    stats->exact_tests = 0;
  }

  Primitive* best = NULL;

  for (size_t i = scene->count; i > 0; i--) {
    Primitive* p = scene->primitives[i - 1];

    // não pode ficar à frente do melhor candidato
    if (best != NULL && p->z_index <= best->z_index) {
      continue;
    }

    if (is_opt) {
      BoundingBox box = primitive_get_aabb(p);
      if (stats) stats->aabb_tests++;
      if (pt.x < box.min.x || pt.x > box.max.x || pt.y < box.min.y ||
          pt.y > box.max.y) {
        continue;
      }
    }

    if (stats) stats->exact_tests++;
    if (primitive_contains_point(p, pt)) {
      best = p;
    }
  }

  return best;
}

// Painter's Algorithm: desenha do fundo para a frente e grava em PPM
bool vector_scene_render(const VectorScene* scene,
                         const char* output_filename) {
  if (!scene || !output_filename) {
    return false;
  }

  int width = scene->width;
  int height = scene->height;

  uint8_t* pixels = malloc((size_t)width * height * 3);
  if (pixels == NULL) {
    return false;
  }
  for (size_t i = 0; i < (size_t)width * height; i++) {
    pixels[3 * i] = scene->bg_color.r;
    pixels[3 * i + 1] = scene->bg_color.g;
    pixels[3 * i + 2] = scene->bg_color.b;
  }

  // a cena é const, por isso ordena-se uma cópia do array
  Primitive** order = NULL;
  if (scene->count > 0) {
    order = malloc(scene->count * sizeof(Primitive*));
    if (order == NULL) {
      free(pixels);
      return false;
    }
    memcpy(order, scene->primitives, scene->count * sizeof(Primitive*));
    if (!merge_sort_array(order, scene->count, NULL)) {
      free(order);
      free(pixels);
      return false;
    }
  }

  for (size_t k = 0; k < scene->count; k++) {
    Primitive* p = order[k];
    BoundingBox box = primitive_get_aabb(p);

    // janela de píxeis da AABB, cortada ao canvas
    double fx0 = fmax(0.0, floor(box.min.x));
    double fy0 = fmax(0.0, floor(box.min.y));
    double fx1 = fmin((double)(width - 1), ceil(box.max.x));
    double fy1 = fmin((double)(height - 1), ceil(box.max.y));
    if (fx0 > fx1 || fy0 > fy1) {
      continue;
    }
    int x0 = (int)fx0, y0 = (int)fy0, x1 = (int)fx1, y1 = (int)fy1;

    Color c = primitive_get_color(p);
    double a = primitive_get_alpha(p);

    for (int y = y0; y <= y1; y++) {
      for (int x = x0; x <= x1; x++) {
        // testa o centro do píxel
        Point center = {x + 0.5, y + 0.5};
        if (!primitive_contains_point(p, center)) {
          continue;
        }
        // alpha blending: C_final = a * C_nova + (1 - a) * C_fundo
        uint8_t* px = &pixels[((size_t)y * width + x) * 3];
        px[0] = (uint8_t)(a * c.r + (1.0 - a) * px[0]);
        px[1] = (uint8_t)(a * c.g + (1.0 - a) * px[1]);
        px[2] = (uint8_t)(a * c.b + (1.0 - a) * px[2]);
      }
    }
  }

  bool ok = save_ppm_image(output_filename, width, height, pixels);
  free(order);
  free(pixels);
  return ok;
}

bool vector_scene_save_text(const VectorScene* scene, const char* filename) {
  if (!scene || !filename) {
    return false;
  }

  FILE* file = fopen(filename, "w");
  if (!file) {
    return false;
  }

  Color bg = scene->bg_color;
  fprintf(file, "SCENE %d %d %d %d %d\n", scene->width, scene->height, bg.r,
          bg.g, bg.b);

  for (size_t i = 0; i < scene->count; i++) {
    Primitive* p = scene->primitives[i];
    if (!p) {
      continue;
    }

    Color c = p->color;

    switch (p->type) {
      case PRIM_CIRCLE:
        fprintf(file, "CIRC %d %d %d %d %d %.3f %.2f %.2f %.2f\n", p->id,
                p->z_index, c.r, c.g, c.b, p->alpha, p->geom.circle.center.x,
                p->geom.circle.center.y, p->geom.circle.radius);
        break;

      case PRIM_RECTANGLE:
        fprintf(file, "RECT %d %d %d %d %d %.3f %.2f %.2f %.2f %.2f\n", p->id,
                p->z_index, c.r, c.g, c.b, p->alpha,
                p->geom.rectangle.top_left.x, p->geom.rectangle.top_left.y,
                p->geom.rectangle.width, p->geom.rectangle.height);
        break;

      case PRIM_SEGMENT:
        fprintf(file, "SEG %d %d %d %d %d %.3f %.2f %.2f %.2f %.2f\n", p->id,
                p->z_index, c.r, c.g, c.b, p->alpha, p->geom.segment.p1.x,
                p->geom.segment.p1.y, p->geom.segment.p2.x,
                p->geom.segment.p2.y);
        break;

      case PRIM_POLYGON:
        fprintf(file, "POLY %d %d %d %d %d %.3f %zu", p->id, p->z_index, c.r,
                c.g, c.b, p->alpha, p->geom.polygon.num_vertices);

        for (size_t v = 0; v < p->geom.polygon.num_vertices; v++) {
          fprintf(file, " %.2f %.2f", p->geom.polygon.vertices[v].x,
                  p->geom.polygon.vertices[v].y);
        }
        fprintf(file, "\n");
        break;
    }
  }

  fclose(file);
  return true;
}

VectorScene* vector_scene_load_text(const char* filename) {
  if (!filename) {
    return NULL;
  }

  FILE* file = fopen(filename, "r");
  if (!file) {
    return NULL;
  }

  char line[1024];
  VectorScene* scene = NULL;

  while (fgets(line, sizeof(line), file)) {
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
      continue;
    }

    char tag[16];
    if (sscanf(line, "%15s", tag) != 1) {
      continue;
    }

    if (strcmp(tag, "SCENE") == 0) {
      int w, h, r, g, b;
      if (sscanf(line, "SCENE %d %d %d %d %d", &w, &h, &r, &g, &b) == 5) {
        Color bg = {(uint8_t)r, (uint8_t)g, (uint8_t)b};
        scene = vector_scene_create(w, h, bg);
      }
    } else if (scene != NULL) {
      int id, z_index, r, g, b;
      double alpha;

      if (strcmp(tag, "CIRC") == 0) {
        double cx, cy, radius;
        if (sscanf(line, "CIRC %d %d %d %d %d %lf %lf %lf %lf", &id, &z_index,
                   &r, &g, &b, &alpha, &cx, &cy, &radius) == 9) {
          Point center = {cx, cy};
          Color color = {(uint8_t)r, (uint8_t)g, (uint8_t)b};
          Primitive* p = primitive_create_circle(id, center, radius, color,
                                                 alpha, z_index);
          if (p) {
            vector_scene_add_primitive(scene, p);
          }
        }
      } else if (strcmp(tag, "RECT") == 0) {
        double x, y, w, h;
        if (sscanf(line, "RECT %d %d %d %d %d %lf %lf %lf %lf %lf", &id,
                   &z_index, &r, &g, &b, &alpha, &x, &y, &w, &h) == 10) {
          Point top_left = {x, y};
          Color color = {(uint8_t)r, (uint8_t)g, (uint8_t)b};
          Primitive* p = primitive_create_rectangle(id, top_left, w, h, color,
                                                    alpha, z_index);
          if (p) {
            vector_scene_add_primitive(scene, p);
          }
        }
      } else if (strcmp(tag, "SEG") == 0) {
        double x1, y1, x2, y2;
        if (sscanf(line, "SEG %d %d %d %d %d %lf %lf %lf %lf %lf", &id,
                   &z_index, &r, &g, &b, &alpha, &x1, &y1, &x2, &y2) == 10) {
          Point p1 = {x1, y1};
          Point p2 = {x2, y2};
          Color color = {(uint8_t)r, (uint8_t)g, (uint8_t)b};
          Primitive* p =
              primitive_create_segment(id, p1, p2, color, alpha, z_index);
          if (p) {
            vector_scene_add_primitive(scene, p);
          }
        }
      } else if (strcmp(tag, "POLY") == 0) {
        size_t num_verts;
        int read_header = sscanf(line, "POLY %d %d %d %d %d %lf %zu", &id,
                                 &z_index, &r, &g, &b, &alpha, &num_verts);
        if (read_header == 7) {
          Color color = {(uint8_t)r, (uint8_t)g, (uint8_t)b};
          Primitive* p = primitive_create_polygon(id, color, alpha, z_index);
          if (p) {
            char* ptr = line;
            // salta os 8 campos do cabeçalho POLY (o código original saltava só 7)
            for (int k = 0; k < 8; k++) {
              while (*ptr && *ptr != ' ' && *ptr != '\t') {
                ptr++;
              }
              while (*ptr && (*ptr == ' ' || *ptr == '\t')) {
                ptr++;
              }
            }

            for (size_t v = 0; v < num_verts; v++) {
              double vx, vy;
              if (sscanf(ptr, "%lf %lf", &vx, &vy) == 2) {
                Point pt = {vx, vy};
                polygon_add_vertex(p, pt);

                for (int k = 0; k < 2; k++) {
                  while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n' &&
                         *ptr != '\r') {
                    ptr++;
                  }
                  while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' ||
                                  *ptr == '\r')) {
                    ptr++;
                  }
                }
              }
            }
            vector_scene_add_primitive(scene, p);
          }
        }
      }
    }
  }

  fclose(file);
  return scene;
}
