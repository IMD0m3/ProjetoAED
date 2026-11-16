/**
 * @file benchmark.c
 * @brief Benchmarks de ordenação por z_index e de picking (CSV para stdout).
 * Uso: ./benchmark
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "vector_scene.h"

#define W 1000
#define H 1000

/* Cena com n círculos pequenos (raio 5..20) em posições e z aleatórios. */
static VectorScene* make_scene(int n, int z_mode) {
  VectorScene* s = vector_scene_create(W, H, (Color){255, 255, 255});
  for (int i = 0; i < n; i++) {
    Point c = {rand() % W, rand() % H};
    int z = (z_mode == 0) ? rand() % 1000000    /* aleatório */
            : (z_mode == 1) ? i                 /* já ordenado */
                            : n - i;            /* ordem inversa */
    Primitive* p = primitive_create_circle(
        -1, c, 5 + rand() % 16, (Color){rand() % 256, rand() % 256, 0}, 1.0, z);
    vector_scene_add_primitive(s, p);
  }
  return s;
}

static double now_ms(void) {
  return (double)clock() * 1000.0 / CLOCKS_PER_SEC;
}


/* Cena com n polígonos de 32 vértices (aproximam círculos): o teste exato
 * (ray-casting) custa O(32), muito mais que um teste de AABB. */
static VectorScene* make_poly_scene(int n) {
  VectorScene* s = vector_scene_create(W, H, (Color){255, 255, 255});
  for (int i = 0; i < n; i++) {
    double cx = rand() % W, cy = rand() % H, r = 5 + rand() % 16;
    Primitive* p = primitive_create_polygon(-1, (Color){0, 100, 200}, 1.0,
                                            rand() % 1000000);
    for (int v = 0; v < 32; v++) {
      double ang = 2.0 * 3.14159265358979 * v / 32.0;
      polygon_add_vertex(p, (Point){cx + r * cos(ang), cy + r * sin(ang)});
    }
    vector_scene_add_primitive(s, p);
  }
  return s;
}

/* M picks em pontos aleatórios com as duas estratégias; compara resultados. */
static void bench_pick(const char* label, VectorScene* s, int n) {
  const int M = 1000;
  vector_scene_sort_by_z(s, true, NULL);
  Point* pts = malloc(M * sizeof(Point));
  Primitive** res_d = malloc(M * sizeof(Primitive*));
  for (int i = 0; i < M; i++) pts[i] = (Point){rand() % W, rand() % H};

  unsigned long a_d = 0, e_d = 0, a_o = 0, e_o = 0;
  int diff = 0;
  PickStats ps;
  double t0 = now_ms();
  for (int i = 0; i < M; i++) {
    res_d[i] = vector_scene_pick_top_primitive(s, pts[i], false, &ps);
    a_d += ps.aabb_tests; e_d += ps.exact_tests;
  }
  double t1 = now_ms();
  for (int i = 0; i < M; i++) {
    Primitive* r = vector_scene_pick_top_primitive(s, pts[i], true, &ps);
    a_o += ps.aabb_tests; e_o += ps.exact_tests;
    if (r != res_d[i]) diff++;
  }
  double t2 = now_ms();
  printf("PICK,%s,%d,%d,%lu,%lu,%lu,%lu,%.3f,%.3f,%d\n", label, n, M, a_d, e_d,
         a_o, e_o, t1 - t0, t2 - t1, diff);
  free(res_d); free(pts);
}

int main(void) {
  srand(12345);
  int sizes[] = {100, 500, 1000, 5000, 10000, 50000, 100000};
  const char* modes[] = {"aleatorio", "ordenado", "inverso"};

  printf("# SORT: N,caso,algoritmo,comparacoes,movimentos,tempo_ms\n");
  for (size_t k = 0; k < sizeof sizes / sizeof sizes[0]; k++) {
    for (int m = 0; m < 3; m++) {
      for (int fast = 0; fast < 2; fast++) {
        srand(1);
        VectorScene* s = make_scene(sizes[k], m);
        SortStats st;
        double t0 = now_ms();
        vector_scene_sort_by_z(s, fast, &st);
        double t1 = now_ms();
        printf("SORT,%d,%s,%s,%lu,%lu,%.3f\n", sizes[k], modes[m],
               fast ? "merge" : "insertion", st.comparisons, st.swaps, t1 - t0);
        vector_scene_destroy(s);
      }
    }
  }

  printf("# PICK: tipo,N,M,aabb_direto,exact_direto,aabb_opt,exact_opt,"
         "tempo_direto_ms,tempo_opt_ms,diferencas\n");
  for (size_t k = 0; k < sizeof sizes / sizeof sizes[0]; k++) {
    srand(1);
    VectorScene* s = make_scene(sizes[k], 0);
    bench_pick("circulos", s, sizes[k]);
    vector_scene_destroy(s);
  }
  int poly_sizes[] = {100, 1000, 10000, 100000};
  for (size_t k = 0; k < sizeof poly_sizes / sizeof poly_sizes[0]; k++) {
    srand(1);
    VectorScene* s = make_poly_scene(poly_sizes[k]);
    bench_pick("poligonos32", s, poly_sizes[k]);
    vector_scene_destroy(s);
  }
  return 0;
}
