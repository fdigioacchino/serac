// Copyright (c) 2019-2022, Lawrence Livermore National Security, LLC and
// other Serac Project Developers. See the top-level LICENSE file for
// details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#include <fstream>

#include "axom/slic/core/SimpleLogger.hpp"
#include <gtest/gtest.h>
#include "mfem.hpp"

#include "serac/serac_config.hpp"
#include "serac/mesh/mesh_utils.hpp"
#include "serac/physics/state/state_manager.hpp"
#include "serac/physics/solid_functional.hpp"
#include "serac/physics/materials/solid_functional_material.hpp"
#include "serac/physics/materials/parameterized_solid_functional_material.hpp"
#include "serac/infrastructure/initialize.hpp"

namespace serac {

template <int dim>
void functional_solid_test_slide_wall(double expected_disp_norm)
{
  MPI_Barrier(MPI_COMM_WORLD);

  int serial_refinement   = 0;
  int parallel_refinement = 0;

  constexpr int p = 1;

  // Create DataStore
  axom::sidre::DataStore datastore;
  serac::StateManager::initialize(datastore, "solid_functional_slide_wall");

  static_assert(dim == 2 || dim == 3, "Dimension must be 2 or 3 for solid functional slide wall test");

  // Construct the appropriate dimension mesh and give it to the data store
  std::string filename = (dim == 2) ? SERAC_REPO_DIR "/data/meshes/beam-quad-angle.mesh"
                                    : SERAC_REPO_DIR "/data/meshes/beam-hex-angle.mesh";

  auto mesh = mesh::refineAndDistribute(buildMeshFromFile(filename), serial_refinement, parallel_refinement);
  serac::StateManager::setMesh(std::move(mesh));

  // Define a boundary attribute set
  std::set<int> ess_bdr = {1};

  // Construct a functional-based solid mechanics solver
  SolidFunctional<p, dim> solid_solver(solid_mechanics::default_static_options, GeometricNonlinearities::On, 
                                       "solid_functional_slide_wall");

  // Set the material parameters
  solid_mechanics::NeoHookean<dim> mat{1.0, 1.0, 1.0};
  solid_solver.setMaterial(mat);

  // Define the function for the initial displacement and boundary condition
  auto bc = [](const mfem::Vector&, mfem::Vector& bc_vec) -> void { bc_vec = 0.0; };

  // Set the initial displacement and boundary condition
  solid_solver.setDisplacementBCs(ess_bdr, bc);
  solid_solver.setDisplacement(bc);

  // Set the sliding wall boundary condition on attribute 2
  solid_solver.setSlideWallBCs({2});

  // Set a constant uniform body forms
  tensor<double, dim> constant_force;

  constant_force[0] = 0.0;
  constant_force[1] = 5.0e-4;

  if (dim == 3) {
    constant_force[2] = 0.0;
  }

  solid_mechanics::ConstantBodyForce<dim> force{constant_force};
  solid_solver.addBodyForce(force);

  // Finalize the data structures
  solid_solver.completeSetup();

  // Perform the quasi-static solve
  double dt = 1.0;
  solid_solver.advanceTimestep(dt);

  // Output the sidre-based and paraview plot files
  solid_solver.outputState("paraview_output");

  // Check the final displacement norm
  EXPECT_NEAR(expected_disp_norm, norm(solid_solver.displacement()), 1.0e-6);

  auto [my_rank, num_ranks] = getMPIInfo();

  if (my_rank == 1) {
    if constexpr (dim == 2) {
      tensor<double, dim> normal{{0.894427, 0.447214}};

      tensor<double, dim> end_displacement;

      int end_node_index = solid_solver.displacement().Size() - dim;

      end_displacement[0] = solid_solver.displacement()(end_node_index);
      end_displacement[1] = solid_solver.displacement()(end_node_index + 1);

      // Ensure the beam end displacement is orthogonal to the wall normal
      EXPECT_NEAR(dot(normal, end_displacement), 0.0, 5.0e-7);
    }

    if constexpr (dim == 3) {
      tensor<double, dim> normal{{0.816497, -0.408248, -0.408248}};

      int end_node_index = solid_solver.displacement().Size() - dim;

      tensor<double, dim> end_displacement;

      end_displacement[0] = solid_solver.displacement()(end_node_index);
      end_displacement[1] = solid_solver.displacement()(end_node_index + 1);
      end_displacement[2] = solid_solver.displacement()(end_node_index + 2);

      // Ensure the beam end displacement is orthogonal to the wall normal
      EXPECT_NEAR(dot(normal, end_displacement), 0.0, 5.0e-7);
    }
  }
}

TEST(SolidFunctional, 2DSlideWall) { functional_solid_test_slide_wall<2>(0.51146965994291915); }
TEST(SolidFunctional, 3DSlideWall) { functional_solid_test_slide_wall<3>(0.41503317976248277); }

}  // namespace serac

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  MPI_Init(&argc, &argv);

  axom::slic::SimpleLogger logger;

  int result = RUN_ALL_TESTS();
  MPI_Finalize();

  return result;
}
