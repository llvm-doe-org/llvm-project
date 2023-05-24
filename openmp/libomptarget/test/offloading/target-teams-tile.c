// Check that omp tile (introduced in OpenMP 5.1) is permitted and behaves when
// strictly nested within omp target teams.  This is an extension to OpenMP 5.2
// and is enabled by default.

// RUN: %libomptarget-compile-generic -fopenmp-version=51
// RUN: %libomptarget-run-generic 2>&1 | %fcheck-generic

#include <omp.h>
#include <stdio.h>

#define NUM_TEAMS_UPPER 256
#define I_NTILES 8
#define J_NTILES 9
#define I_NELEMS 2
#define J_NELEMS 3

int main() {
  int numTeams;
  int order[NUM_TEAMS_UPPER][I_NTILES][J_NTILES][I_NELEMS][J_NELEMS];
  #pragma omp target teams num_teams(NUM_TEAMS_UPPER) map(from : numTeams)
  {
    int team = omp_get_team_num();
    if (team == 0)
      numTeams = omp_get_num_teams();
    int next = 0;
    #pragma omp tile sizes(I_NELEMS, J_NELEMS)
    for (int i = 0; i < I_NTILES * I_NELEMS; ++i) {
      for (int j = 0; j < J_NTILES * J_NELEMS; ++j) {
        int iTile = i / I_NELEMS;
        int jTile = j / J_NELEMS;
        int iElem = i % I_NELEMS;
        int jElem = j % J_NELEMS;
        order[team][iTile][jTile][iElem][jElem] = next++;
      }
    }
  }
  printf("numTeams = %d\n", numTeams);
  for (int team = 0; team < numTeams; ++team) {
    int expected = 0;
    for (int iTile = 0; iTile < I_NTILES; ++iTile) {
      for (int jTile = 0; jTile < J_NTILES; ++jTile) {
        for (int iElem = 0; iElem < I_NELEMS; ++iElem) {
          for (int jElem = 0; jElem < J_NELEMS; ++jElem) {
            int actual = order[team][iTile][jTile][iElem][jElem];
            if (expected != actual) {
              printf("error: order[%d][%d][%d][%d][%d] = %d, expected %d\n",
                     team, iTile, jTile, iElem, jElem, actual, expected);
              return 1;
            }
            ++expected;
          }
        }
      }
    }
  }
  // CHECK: success
  printf("success\n");
  return 0;
}
