/**
 * @file art_generator.c
 * @brief Gera a composição artística do grupo: "Farol numa noite de luar".
 * Cria a cena com o TAD VectorScene, guarda o .vsc e renderiza o .ppm.
 */
#include <stdio.h>
#include <stdlib.h>

#include "vector_scene.h"

static void add(VectorScene* s, Primitive* p) { vector_scene_add_primitive(s, p); }

static Primitive* poly(Color c, double a, int z, int n, const double* xy) {
  Primitive* p = primitive_create_polygon(-1, c, a, z);
  for (int i = 0; i < n; i++) polygon_add_vertex(p, (Point){xy[2 * i], xy[2 * i + 1]});
  return p;
}

int main(void) {
  srand(2026);
  VectorScene* s = vector_scene_create(800, 600, (Color){8, 12, 38});

  /* z=0: céu em faixas semi-transparentes (efeito de gradiente) */
  for (int i = 0; i < 8; i++)
    add(s, primitive_create_rectangle(-1, (Point){0, i * 50.0}, 800, 50,
                                      (Color){60 + i * 12, 40 + i * 8, 90 + i * 6}, 0.18, 0));

  /* z=1: estrelas (pequenos círculos e cruzes feitas com segmentos) */
  for (int i = 0; i < 70; i++) {
    double x = rand() % 800, y = rand() % 330;
    add(s, primitive_create_circle(-1, (Point){x, y}, 1 + rand() % 2,
                                   (Color){255, 255, 230}, 0.5 + (rand() % 50) / 100.0, 1));
  }
  for (int i = 0; i < 6; i++) {
    double x = 40 + rand() % 720, y = 20 + rand() % 250;
    add(s, primitive_create_segment(-1, (Point){x - 6, y}, (Point){x + 6, y}, (Color){255, 255, 255}, 0.7, 1));
    add(s, primitive_create_segment(-1, (Point){x, y - 6}, (Point){x, y + 6}, (Color){255, 255, 255}, 0.7, 1));
  }

  /* z=2: lua com halo (círculos concêntricos transparentes) */
  for (int r = 130; r >= 70; r -= 20)
    add(s, primitive_create_circle(-1, (Point){620, 110}, r, (Color){200, 210, 255}, 0.07, 2));
  add(s, primitive_create_circle(-1, (Point){620, 110}, 42, (Color){245, 240, 210}, 1.0, 3));
  add(s, primitive_create_circle(-1, (Point){608, 100}, 8, (Color){215, 210, 185}, 0.6, 3));
  add(s, primitive_create_circle(-1, (Point){634, 122}, 5, (Color){215, 210, 185}, 0.6, 3));

  /* z=4: montanhas distantes (polígonos) */
  const double m1[] = {0, 380, 90, 300, 170, 350, 260, 270, 380, 380};
  const double m2[] = {330, 380, 450, 290, 540, 340, 650, 260, 800, 380};
  add(s, poly((Color){30, 30, 70}, 0.9, 4, 5, m1));
  add(s, poly((Color){35, 35, 80}, 0.9, 4, 5, m2));

  /* z=5: mar */
  add(s, primitive_create_rectangle(-1, (Point){0, 380}, 800, 220, (Color){14, 40, 85}, 1.0, 5));
  for (int i = 0; i < 10; i++)  /* reflexo da lua: faixas cada vez mais curtas */
    add(s, primitive_create_rectangle(-1, (Point){620 - 50 + i * 4, 385 + i * 20.0},
                                      100 - i * 8, 6, (Color){230, 230, 200}, 0.35, 6));
  for (int i = 0; i < 40; i++) {  /* ondas */
    double x = rand() % 780, y = 400 + rand() % 190;
    add(s, primitive_create_segment(-1, (Point){x, y}, (Point){x + 15 + rand() % 20, y},
                                    (Color){150, 190, 230}, 0.3, 6));
  }

  /* z=7: rochedo do farol */
  const double rock[] = {20, 480, 70, 400, 130, 385, 230, 395, 290, 440, 330, 480};
  add(s, poly((Color){28, 28, 36}, 1.0, 7, 6, rock));

  /* z=8: torre do farol com listras, galeria, lanterna e telhado */
  add(s, primitive_create_rectangle(-1, (Point){150, 220}, 70, 175, (Color){235, 235, 235}, 1.0, 8));
  for (int i = 0; i < 3; i++)
    add(s, primitive_create_rectangle(-1, (Point){150, 245 + i * 55.0}, 70, 25, (Color){190, 30, 40}, 1.0, 9));
  add(s, primitive_create_rectangle(-1, (Point){140, 205}, 90, 15, (Color){50, 50, 60}, 1.0, 9));
  add(s, primitive_create_rectangle(-1, (Point){160, 160}, 50, 45, (Color){255, 225, 120}, 0.95, 9));
  const double roof[] = {152, 160, 218, 160, 185, 125};
  add(s, poly((Color){190, 30, 40}, 1.0, 9, 3, roof));
  add(s, primitive_create_rectangle(-1, (Point){175, 345, }, 20, 50, (Color){60, 40, 30}, 1.0, 9));

  /* z=10: feixes de luz (polígonos amarelos transparentes + raios) */
  const double beamR[] = {210, 175, 800, 90, 800, 250};
  const double beamL[] = {160, 175, 0, 120, 0, 230};
  add(s, poly((Color){255, 240, 150}, 0.22, 10, 3, beamR));
  add(s, poly((Color){255, 240, 150}, 0.14, 10, 3, beamL));
  for (int i = 0; i < 5; i++)
    add(s, primitive_create_segment(-1, (Point){210, 180}, (Point){800, 110 + i * 35.0},
                                    (Color){255, 245, 180}, 0.35, 11));
  add(s, primitive_create_circle(-1, (Point){185, 182}, 55, (Color){255, 235, 140}, 0.12, 10));
  add(s, primitive_create_circle(-1, (Point){185, 182}, 30, (Color){255, 235, 140}, 0.25, 10));

  /* z=12: barco à vela (casco e velas poligonais, mastro em segmento) */
  const double hull[] = {470, 500, 560, 500, 545, 525, 485, 525};
  const double sail1[] = {515, 498, 515, 430, 555, 498};
  const double sail2[] = {509, 498, 509, 445, 480, 498};
  add(s, poly((Color){90, 55, 35}, 1.0, 12, 4, hull));
  add(s, primitive_create_segment(-1, (Point){512, 500}, (Point){512, 422}, (Color){70, 45, 30}, 1.0, 12));
  add(s, poly((Color){240, 235, 220}, 0.95, 12, 3, sail1));
  add(s, poly((Color){220, 215, 200}, 0.9, 12, 3, sail2));
  /* sombra do barco no mar (transparente, por baixo do casco) */
  add(s, primitive_create_rectangle(-1, (Point){480, 526}, 70, 8, (Color){0, 0, 20}, 0.35, 11));

  /* nuvens finas à frente da lua */
  for (int i = 0; i < 3; i++)
    add(s, primitive_create_circle(-1, (Point){560 + i * 45.0, 135 - (i % 2) * 8}, 26 + i * 4,
                                   (Color){120, 120, 170}, 0.4, 13));

  printf("Primitivas: %zu\n", vector_scene_get_count(s));
  vector_scene_save_text(s, "art_group_XX_YY.vsc");
  if (!vector_scene_render(s, "art_group_XX_YY.ppm")) {
    fprintf(stderr, "erro a renderizar\n");
    return 1;
  }
  vector_scene_destroy(s);
  return 0;
}
