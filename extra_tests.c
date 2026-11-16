/**
 * @file extra_tests.c
 * @brief Testes adicionais (próprios do grupo) para Primitive e VectorScene.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "primitive_internal.h"
#include "vector_scene.h"

static int run = 0, failed = 0;
#define CHECK(cond, msg)                                         \
  do {                                                           \
    run++;                                                       \
    if (!(cond)) {                                               \
      failed++;                                                  \
      printf("  [FAIL] linha %d: %s\n", __LINE__, msg);          \
    }                                                            \
  } while (0)

static const Color RED = {255, 0, 0};

static void test_polygon_growth(void) {
  Primitive* poly = primitive_create_polygon(1, RED, 1.0, 0);
  for (int i = 0; i < 100; i++) {  // força vários realloc
    CHECK(polygon_add_vertex(poly, (Point){i, i * 2.0}), "add vertex");
  }
  CHECK(polygon_get_num_vertices(poly) == 100, "100 vértices");
  CHECK(poly->geom.polygon.capacity >= 100, "capacidade cresceu");
  CHECK(poly->geom.polygon.vertices[99].y == 198.0, "último vértice");

  CHECK(polygon_remove_vertex(poly, 0), "remover primeiro");
  CHECK(poly->geom.polygon.vertices[0].x == 1.0, "shift do primeiro");
  CHECK(polygon_remove_vertex(poly, 98), "remover último");
  CHECK(polygon_get_num_vertices(poly) == 98, "98 vértices");
  CHECK(!polygon_remove_vertex(poly, 98), "índice fora dos limites");

  Primitive* circ = primitive_create_circle(2, (Point){0, 0}, 1, RED, 1, 0);
  CHECK(!polygon_add_vertex(circ, (Point){1, 1}), "add vertex em círculo");
  CHECK(!polygon_add_vertex(NULL, (Point){1, 1}), "add vertex em NULL");
  primitive_destroy(circ);
  primitive_destroy(poly);
}

static void test_aabb_and_contains(void) {
  Primitive* r = primitive_create_rectangle(1, (Point){10, 20}, 30, 40, RED, 1, 0);
  BoundingBox b = primitive_get_aabb(r);
  CHECK(b.min.x == 10 && b.min.y == 20 && b.max.x == 40 && b.max.y == 60,
        "AABB retângulo");
  CHECK(primitive_contains_point(r, (Point){25, 30}), "retângulo contém");
  CHECK(primitive_contains_point(r, (Point){10, 20}), "retângulo: fronteira");
  CHECK(!primitive_contains_point(r, (Point){41, 30}), "retângulo não contém");
  primitive_destroy(r);

  Primitive* s = primitive_create_segment(2, (Point){0, 0}, (Point){100, 0}, RED, 1, 0);
  CHECK(primitive_contains_point(s, (Point){50, 0.5}), "segmento perto");
  CHECK(!primitive_contains_point(s, (Point){50, 5}), "segmento longe");
  CHECK(!primitive_contains_point(s, (Point){150, 0}), "além da ponta");
  b = primitive_get_aabb(s);
  CHECK(b.min.y < 0 && b.max.y > 0, "AABB do segmento tem folga");
  primitive_destroy(s);

  Primitive* t = primitive_create_polygon(3, RED, 1, 0);
  CHECK(!primitive_contains_point(t, (Point){0, 0}), "polígono vazio");
  polygon_add_vertex(t, (Point){0, 0});
  polygon_add_vertex(t, (Point){10, 0});
  polygon_add_vertex(t, (Point){5, 10});
  b = primitive_get_aabb(t);
  CHECK(b.min.x == 0 && b.max.x == 10 && b.max.y == 10, "AABB triângulo");
  CHECK(primitive_contains_point(t, (Point){5, 3}), "triângulo contém");
  CHECK(!primitive_contains_point(t, (Point){1, 9}), "triângulo não contém");
  polygon_add_vertex(t, (Point){5, 50});  // AABB tem de ser recalculada
  CHECK(t->is_aabb_dirty, "dirty após add");
  b = primitive_get_aabb(t);
  CHECK(b.max.y == 50 && !t->is_aabb_dirty, "AABB recalculada (lazy)");
  primitive_destroy(t);
}

static void test_transforms(void) {
  Primitive* c = primitive_create_circle(1, (Point){10, 10}, 4, RED, 1, 0);
  primitive_scale(c, 2.0);
  CHECK(c->geom.circle.radius == 8.0, "escala círculo");
  primitive_scale(c, -1.0);
  CHECK(c->geom.circle.radius == 8.0, "fator inválido ignorado");
  primitive_destroy(c);

  Primitive* s = primitive_create_segment(2, (Point){0, 0}, (Point){10, 0}, RED, 1, 0);
  primitive_scale(s, 2.0);  // relativo ao ponto médio (5,0)
  CHECK(s->geom.segment.p1.x == -5 && s->geom.segment.p2.x == 15, "escala segmento");
  primitive_destroy(s);

  Primitive* p = primitive_create_polygon(3, RED, 1, 0);
  polygon_add_vertex(p, (Point){0, 0});
  polygon_add_vertex(p, (Point){10, 0});
  polygon_add_vertex(p, (Point){10, 10});
  polygon_add_vertex(p, (Point){0, 10});
  primitive_scale(p, 2.0);  // centro (5,5): passa a ir de -5 a 15
  BoundingBox b = primitive_get_aabb(p);
  CHECK(b.min.x == -5 && b.max.x == 15 && b.min.y == -5 && b.max.y == 15,
        "escala polígono em torno do centro");
  primitive_translate(p, 5, 5);
  b = primitive_get_aabb(p);
  CHECK(b.min.x == 0 && b.max.y == 20, "translação polígono");
  primitive_destroy(p);

  Primitive* r = primitive_create_rectangle(4, (Point){1, 1}, 2, 2, RED, 1, 0);
  CHECK(primitive_create_rectangle(5, (Point){0, 0}, 0, 2, RED, 1, 0) == NULL,
        "retângulo largura 0 rejeitado");
  CHECK(primitive_create_segment(6, (Point){0, 0}, (Point){1, 1}, RED, 1.5, 0) == NULL,
        "alpha inválido rejeitado");
  primitive_destroy(r);
}

static void test_scene_collection(void) {
  VectorScene* s = vector_scene_create(100, 100, RED);
  CHECK(vector_scene_create(0, 10, RED) == NULL, "largura 0 rejeitada");
  for (int i = 0; i < 50; i++) {  // > capacidade inicial (8)
    int id = vector_scene_add_primitive(
        s, primitive_create_circle(-1, (Point){i, i}, 2, RED, 1, 50 - i));
    CHECK(id == i, "IDs automáticos sequenciais");
  }
  CHECK(vector_scene_get_count(s) == 50, "50 primitivas");
  CHECK(vector_scene_add_primitive(s, primitive_create_circle(
            100, (Point){0, 0}, 1, RED, 1, 0)) == 100, "ID explícito mantido");
  int next = vector_scene_add_primitive(
      s, primitive_create_circle(-1, (Point){0, 0}, 1, RED, 1, 0));
  CHECK(next == 101, "ID automático não colide com explícitos");
  CHECK(vector_scene_remove_primitive(s, 0), "remover primeira");
  CHECK(vector_scene_get_primitive_by_id(s, 0) == NULL, "já não existe");
  CHECK(vector_scene_get_primitive_by_index(s, 0)->id == 1, "shift à esquerda");
  CHECK(!vector_scene_remove_primitive(s, 9999), "remover ID inexistente");
  CHECK(vector_scene_translate_primitive(s, 1, 5, 5), "translate por id");
  CHECK(!vector_scene_translate_primitive(s, 9999, 5, 5), "translate id inválido");
  CHECK(vector_scene_set_z_index(s, 1, 77), "set z");
  CHECK(vector_scene_get_primitive_by_id(s, 1)->z_index == 77, "z alterado");
  CHECK(!vector_scene_set_z_index(s, 9999, 1), "set z id inválido");
  vector_scene_destroy(s);
}

static int is_sorted(VectorScene* s) {
  for (size_t i = 1; i < vector_scene_get_count(s); i++)
    if (primitive_get_z_index(vector_scene_get_primitive_by_index(s, i - 1)) >
        primitive_get_z_index(vector_scene_get_primitive_by_index(s, i)))
      return 0;
  return 1;
}

static void test_sorting_and_picking(void) {
  for (int fast = 0; fast < 2; fast++) {
    srand(7);
    VectorScene* s = vector_scene_create(100, 100, RED);
    for (int i = 0; i < 300; i++)
      vector_scene_add_primitive(s, primitive_create_circle(
          -1, (Point){rand() % 100, rand() % 100}, 8, RED, 1, rand() % 20));
    SortStats st;
    vector_scene_sort_by_z(s, fast, &st);
    CHECK(is_sorted(s), "ordenado por z");
    CHECK(vector_scene_get_count(s) == 300, "nenhuma primitiva perdida");
    unsigned long c1 = st.comparisons;
    vector_scene_sort_by_z(s, fast, &st);  // já ordenado: continua certo
    CHECK(is_sorted(s) && st.comparisons > 0, "re-ordenar");
    if (!fast) CHECK(st.comparisons == 299 && st.swaps == 0, "insertion: melhor caso");
    CHECK(c1 > 0, "contou comparações");
    vector_scene_destroy(s);
  }

  // Estabilidade: empates mantêm a ordem de inserção
  VectorScene* s = vector_scene_create(10, 10, RED);
  for (int i = 0; i < 20; i++)
    vector_scene_add_primitive(s, primitive_create_circle(-1, (Point){5, 5}, 1, RED, 1, i % 2));
  vector_scene_sort_by_z(s, true, NULL);
  int stable = 1, last_id = -1, last_z = -1;
  for (size_t i = 0; i < 20; i++) {
    Primitive* p = vector_scene_get_primitive_by_index(s, i);
    if (primitive_get_z_index(p) == last_z && primitive_get_id(p) < last_id) stable = 0;
    last_z = primitive_get_z_index(p); last_id = primitive_get_id(p);
  }
  CHECK(stable, "MergeSort estável");
  vector_scene_destroy(s);

  // Picking: estratégias concordam, mesmo com array NÃO ordenado
  srand(3);
  s = vector_scene_create(100, 100, RED);
  for (int i = 0; i < 200; i++)
    vector_scene_add_primitive(s, primitive_create_rectangle(
        -1, (Point){rand() % 90, rand() % 90}, 10, 10, RED, 1, rand() % 50));
  int mismatch = 0, wrong_top = 0;
  for (int i = 0; i < 500; i++) {
    Point pt = {rand() % 100, rand() % 100};
    PickStats a, b;
    Primitive* d = vector_scene_pick_top_primitive(s, pt, false, &a);
    Primitive* o = vector_scene_pick_top_primitive(s, pt, true, &b);
    if (d != o) mismatch++;
    CHECK(b.exact_tests <= a.exact_tests, "opt nunca faz mais testes exatos");
    CHECK(a.aabb_tests == 0, "direto não usa AABB");
    // verificação por força bruta: o escolhido tem o maior z entre os que contêm pt
    int best_z = -1;
    for (size_t k = 0; k < vector_scene_get_count(s); k++) {
      Primitive* q = vector_scene_get_primitive_by_index(s, k);
      if (primitive_contains_point(q, pt) && primitive_get_z_index(q) > best_z)
        best_z = primitive_get_z_index(q);
    }
    if ((d == NULL && best_z != -1) || (d && primitive_get_z_index(d) != best_z)) wrong_top++;
  }
  CHECK(mismatch == 0, "direto e otimizado dão o mesmo resultado");
  CHECK(wrong_top == 0, "escolhe sempre o z_index mais alto");
  CHECK(vector_scene_pick_top_primitive(s, (Point){-50, -50}, true, NULL) == NULL,
        "ponto fora da cena (stats NULL permitido)");
  vector_scene_destroy(s);
}

static void test_render_and_io(void) {
  VectorScene* s = vector_scene_create(50, 50, (Color){0, 0, 0});
  vector_scene_add_primitive(s, primitive_create_rectangle(
      -1, (Point){0, 0}, 50, 50, (Color){200, 100, 0}, 0.5, 1));
  Primitive* poly = primitive_create_polygon(-1, (Color){0, 255, 0}, 1.0, 2);
  polygon_add_vertex(poly, (Point){-100, -100});  // sai do canvas: clipping
  polygon_add_vertex(poly, (Point){20, -100});
  polygon_add_vertex(poly, (Point){20, 20});
  vector_scene_add_primitive(s, poly);
  CHECK(vector_scene_render(s, "extra_render.ppm"), "render com clipping");
  CHECK(!vector_scene_render(s, "/nao/existe/x.ppm"), "render em caminho inválido");

  // alpha blending: laranja 0.5 sobre preto = (100, 50, 0) no pixel (49,49) (zona sem polígono)
  FILE* f = fopen("extra_render.ppm", "rb");
  CHECK(f != NULL, "abrir ppm");
  if (f) {
    int w, h, mx;
    if (fscanf(f, "P6 %d %d %d", &w, &h, &mx) != 3) { fclose(f); CHECK(0, "cabeçalho ppm"); return; }
    fgetc(f);
    unsigned char px[3];
    // píxel (49,49): fora do triângulo
    fseek(f, ((long)49 * w + 49) * 3, SEEK_CUR);
    CHECK(fread(px, 1, 3, f) == 3, "ler píxel");
    CHECK(px[0] == 100 && px[1] == 50 && px[2] == 0, "alpha blending 50%");
    fclose(f);
  }

  CHECK(vector_scene_save_text(s, "extra_scene.vsc"), "guardar .vsc");
  VectorScene* t = vector_scene_load_text("extra_scene.vsc");
  CHECK(t != NULL && vector_scene_get_count(t) == 2, "carregar .vsc");
  if (t) {
    Primitive* q = vector_scene_get_primitive_by_index(t, 1);
    CHECK(polygon_get_num_vertices(q) == 3, "polígono carregado: 3 vértices");
    CHECK(q->geom.polygon.vertices[0].x == -100.0, "vértices bem lidos");
    vector_scene_destroy(t);
  }
  vector_scene_destroy(s);
}

int main(void) {
  test_polygon_growth();
  test_aabb_and_contains();
  test_transforms();
  test_scene_collection();
  test_sorting_and_picking();
  test_render_and_io();
  printf("EXTRA TESTS: %d/%d passaram\n", run - failed, run);
  return failed ? 1 : 0;
}
