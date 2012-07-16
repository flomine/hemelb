// 
// Copyright (C) University College London, 2007-2012, all rights reserved.
// 
// This file is part of HemeLB and is CONFIDENTIAL. You may not work 
// with, install, use, duplicate, modify, redistribute or share this
// file, or any part thereof, other than as allowed by any agreement
// specifically made by you with University College London.
// 

#ifndef HEMELB_UNITTESTS_LBTESTS_KERNELTESTS_H
#define HEMELB_UNITTESTS_LBTESTS_KERNELTESTS_H

#include <cstring>
#include <sstream>

#include "lb/kernels/Kernels.h"
#include "lb/kernels/rheologyModels/RheologyModels.h"
#include "lb/kernels/momentBasis/DHumieresD3Q15MRTBasis.h"
#include "lb/kernels/momentBasis/DHumieresD3Q19MRTBasis.h"
#include "unittests/lbtests/LbTestsHelper.h"
#include "unittests/FourCubeLatticeData.h"

#include "unittests/helpers/FourCubeBasedTestFixture.h"

namespace hemelb
{
  namespace unittests
  {
    namespace lbtests
    {

      /**
       * Class containing tests for the functionality of the lattice-Boltzmann kernels. This
       * includes the functions for calculating hydrodynamic variables and for performing
       * collisions.
       */
      class KernelTests : public helpers::FourCubeBasedTestFixture
      {
          CPPUNIT_TEST_SUITE (KernelTests);
          CPPUNIT_TEST (TestAnsumaliEntropicCalculationsAndCollision);
          CPPUNIT_TEST (TestChikatamarlaEntropicCalculationsAndCollision);
          CPPUNIT_TEST (TestLBGKCalculationsAndCollision);
          CPPUNIT_TEST (TestLBGKNNCalculationsAndCollision);
          CPPUNIT_TEST (TestMRTConstantRelaxationTimeEqualsLBGK);
          CPPUNIT_TEST (TestD3Q19MRTConstantRelaxationTimeEqualsLBGK);CPPUNIT_TEST_SUITE_END();
        public:
          void setUp()
          {
            bool dummy;
            topology::NetworkTopology::Instance()->Init(0, NULL, &dummy);

            FourCubeBasedTestFixture::setUp();
            entropic = new lb::kernels::EntropicAnsumali<lb::lattices::D3Q15>(initParams);
            lbgk = new lb::kernels::LBGK<lb::lattices::D3Q15>(initParams);

            /*
             *  We need two kernel instances if we want to work with two different sets of data (and keep the computed
             *  values of tau consistent). One to be used with CalculateDensityVelocityFeq and another with CalculateFeq.
             */
            lbgknn0 = new lb::kernels::LBGKNN<lb::kernels::rheologyModels::CarreauYasudaRheologyModel,
                lb::lattices::D3Q15>(initParams);
            lbgknn1 = new lb::kernels::LBGKNN<lb::kernels::rheologyModels::CarreauYasudaRheologyModel,
                lb::lattices::D3Q15>(initParams);

            mrtLbgkEquivalentKernel =
                new lb::kernels::MRT<lb::kernels::momentBasis::DHumieresD3Q15MRTBasis>(initParams);
            mrtLbgkEquivalentKernel19 =
                new lb::kernels::MRT<lb::kernels::momentBasis::DHumieresD3Q19MRTBasis>(initParams);

          }

          void tearDown()
          {
            delete entropic;
            delete lbgk;
            delete lbgknn0;
            delete lbgknn1;
            delete mrtLbgkEquivalentKernel;
            delete mrtLbgkEquivalentKernel19;
            FourCubeBasedTestFixture::tearDown();
          }

          void TestAnsumaliEntropicCalculationsAndCollision()
          {
            // Initialise the original f distribution to something asymmetric.
            distribn_t f_original[lb::lattices::D3Q15::NUMVECTORS];

            LbTestsHelper::InitialiseAnisotropicTestData<lb::lattices::D3Q15>(0, f_original);

            /*
             * Case 0: use the function that calculates density, velocity and
             * f_eq.
             * Case 1: use the function that leaves density and velocity and
             * calculates f_eq.
             */
            lb::kernels::HydroVars<lb::kernels::EntropicAnsumali<lb::lattices::D3Q15> > hydroVars0(f_original);
            lb::kernels::HydroVars<lb::kernels::EntropicAnsumali<lb::lattices::D3Q15> > hydroVars1(f_original);

            // Calculate density, velocity, equilibrium f.
            entropic->CalculateDensityVelocityFeq(hydroVars0, 0);

            // Manually set density and velocity and calculate eqm f.
            hydroVars1.density = 1.0;
            hydroVars1.v_x = 0.4;
            hydroVars1.v_y = 0.5;
            hydroVars1.v_z = 0.6;

            entropic->CalculateFeq(hydroVars1, 1);

            // Calculate expected values in both cases.
            distribn_t expectedDensity0 = 12.0; // (sum 1 to 15) / 10
            distribn_t expectedDensity1 = 1.0; // Should be unchanged

            distribn_t expectedVelocity0[3];
            LbTestsHelper::CalculateVelocity<lb::lattices::D3Q15>(hydroVars0.f, expectedVelocity0);
            distribn_t expectedVelocity1[3] = { 0.4, 0.5, 0.6 };

            distribn_t expectedFEq0[lb::lattices::D3Q15::NUMVECTORS];
            LbTestsHelper::CalculateAnsumaliEntropicEqmF<lb::lattices::D3Q15>(expectedDensity0,
                                                                              expectedVelocity0[0],
                                                                              expectedVelocity0[1],
                                                                              expectedVelocity0[2],
                                                                              expectedFEq0);
            distribn_t expectedFEq1[lb::lattices::D3Q15::NUMVECTORS];
            LbTestsHelper::CalculateAnsumaliEntropicEqmF<lb::lattices::D3Q15>(expectedDensity1,
                                                                              expectedVelocity1[0],
                                                                              expectedVelocity1[1],
                                                                              expectedVelocity1[2],
                                                                              expectedFEq1);

            // Now compare the expected and actual values in both cases.
            distribn_t allowedError = 1e-10;

            LbTestsHelper::CompareHydros(expectedDensity0,
                                         expectedVelocity0[0],
                                         expectedVelocity0[1],
                                         expectedVelocity0[2],
                                         expectedFEq0,
                                         "Entropic, case 0",
                                         hydroVars0,
                                         allowedError);

            LbTestsHelper::CompareHydros(expectedDensity1,
                                         expectedVelocity1[0],
                                         expectedVelocity1[1],
                                         expectedVelocity1[2],
                                         expectedFEq1,
                                         "Entropic, case 1",
                                         hydroVars1,
                                         allowedError);

            // Do the collision and test the result.
            entropic->DoCollide(lbmParams, hydroVars0);
            entropic->DoCollide(lbmParams, hydroVars1);

            // Get the expected post-collision densities.
            distribn_t expectedPostCollision0[lb::lattices::D3Q15::NUMVECTORS];
            distribn_t expectedPostCollision1[lb::lattices::D3Q15::NUMVECTORS];

            LbTestsHelper::CalculateEntropicCollision<lb::lattices::D3Q15>(f_original,
                                                                           hydroVars0.GetFEq().f,
                                                                           lbmParams->GetTau(),
                                                                           lbmParams->GetBeta(),
                                                                           expectedPostCollision0);

            LbTestsHelper::CalculateEntropicCollision<lb::lattices::D3Q15>(f_original,
                                                                           hydroVars1.GetFEq().f,
                                                                           lbmParams->GetTau(),
                                                                           lbmParams->GetBeta(),
                                                                           expectedPostCollision1);

            // Compare.
            for (unsigned int ii = 0; ii < lb::lattices::D3Q15::NUMVECTORS; ++ii)
            {
              std::stringstream message("Post-collision ");
              message << ii;

              CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(),
                                                   hydroVars0.GetFPostCollision()[ii],
                                                   expectedPostCollision0[ii],
                                                   allowedError);

              CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(),
                                                   hydroVars1.GetFPostCollision()[ii],
                                                   expectedPostCollision1[ii],
                                                   allowedError);
            }
          }

          void TestChikatamarlaEntropicCalculationsAndCollision()
          {
            lb::kernels::EntropicChik<lb::lattices::D3Q15> kernel(initParams);

            // Initialise the original f distribution to something asymmetric.
            distribn_t f_original[lb::lattices::D3Q15::NUMVECTORS];

            LbTestsHelper::InitialiseAnisotropicTestData<lb::lattices::D3Q15>(0, f_original);

            /*
             * Case 0: use the function that calculates density, velocity and
             * f_eq.
             * Case 1: use the function that leaves density and velocity and
             * calculates f_eq.
             */
            lb::kernels::HydroVars<lb::kernels::EntropicChik<lb::lattices::D3Q15> > hydroVars0(f_original);
            lb::kernels::HydroVars<lb::kernels::EntropicChik<lb::lattices::D3Q15> > hydroVars1(f_original);

            // Calculate density, velocity, equilibrium f.
            kernel.CalculateDensityVelocityFeq(hydroVars0, 0);

            // Manually set density and velocity and calculate eqm f.
            hydroVars1.density = 1.0;
            hydroVars1.v_x = 0.4;
            hydroVars1.v_y = 0.5;
            hydroVars1.v_z = 0.6;

            kernel.CalculateFeq(hydroVars1, 1);

            // Calculate expected values in both cases.
            distribn_t expectedDensity0 = 12.0; // (sum 1 to 15) / 10
            distribn_t expectedDensity1 = 1.0; // Should be unchanged

            distribn_t expectedVelocity0[3];
            LbTestsHelper::CalculateVelocity<lb::lattices::D3Q15>(hydroVars0.f, expectedVelocity0);
            distribn_t expectedVelocity1[3] = { 0.4, 0.5, 0.6 };

            distribn_t expectedFEq0[lb::lattices::D3Q15::NUMVECTORS];
            lb::lattices::D3Q15::CalculateEntropicFeqChik(expectedDensity0,
                                                          expectedVelocity0[0],
                                                          expectedVelocity0[1],
                                                          expectedVelocity0[2],
                                                          expectedFEq0);

            distribn_t expectedFEq1[lb::lattices::D3Q15::NUMVECTORS];
            lb::lattices::D3Q15::CalculateEntropicFeqChik(expectedDensity1,
                                                          expectedVelocity1[0],
                                                          expectedVelocity1[1],
                                                          expectedVelocity1[2],
                                                          expectedFEq1);

            // Now compare the expected and actual values in both cases.
            distribn_t allowedError = 1e-10;

            LbTestsHelper::CompareHydros(expectedDensity0,
                                         expectedVelocity0[0],
                                         expectedVelocity0[1],
                                         expectedVelocity0[2],
                                         expectedFEq0,
                                         "Chikatamarla Entropic, case 0",
                                         hydroVars0,
                                         allowedError);

            LbTestsHelper::CompareHydros(expectedDensity1,
                                         expectedVelocity1[0],
                                         expectedVelocity1[1],
                                         expectedVelocity1[2],
                                         expectedFEq1,
                                         "Chikatamarla Entropic, case 1",
                                         hydroVars1,
                                         allowedError);

            // Do the collision and test the result.
            kernel.DoCollide(lbmParams, hydroVars0);
            kernel.DoCollide(lbmParams, hydroVars1);

            // Get the expected post-collision densities.
            distribn_t expectedPostCollision0[lb::lattices::D3Q15::NUMVECTORS];
            distribn_t expectedPostCollision1[lb::lattices::D3Q15::NUMVECTORS];

            LbTestsHelper::CalculateEntropicCollision<lb::lattices::D3Q15>(f_original,
                                                                           hydroVars0.GetFEq().f,
                                                                           lbmParams->GetTau(),
                                                                           lbmParams->GetBeta(),
                                                                           expectedPostCollision0);

            LbTestsHelper::CalculateEntropicCollision<lb::lattices::D3Q15>(f_original,
                                                                           hydroVars1.GetFEq().f,
                                                                           lbmParams->GetTau(),
                                                                           lbmParams->GetBeta(),
                                                                           expectedPostCollision1);

            // Compare.
            for (unsigned int ii = 0; ii < lb::lattices::D3Q15::NUMVECTORS; ++ii)
            {
              std::stringstream message("Post-collision ");
              message << ii;

              CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(),
                                                   hydroVars0.GetFPostCollision()[ii],
                                                   expectedPostCollision0[ii],
                                                   allowedError);

              CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(),
                                                   hydroVars1.GetFPostCollision()[ii],
                                                   expectedPostCollision1[ii],
                                                   allowedError);
            }
          }

          void TestLBGKCalculationsAndCollision()
          {
            // Initialise the original f distribution to something asymmetric.
            distribn_t f_original[lb::lattices::D3Q15::NUMVECTORS];

            LbTestsHelper::InitialiseAnisotropicTestData<lb::lattices::D3Q15>(0, f_original);

            /*
             * Case 0: test the kernel function for calculating density, velocity
             * and f_eq.
             * Case 1: test the function that uses a given density and velocity, and
             * calculates f_eq.
             */
            lb::kernels::HydroVars<lb::kernels::LBGK<lb::lattices::D3Q15> > hydroVars0(f_original);
            lb::kernels::HydroVars<lb::kernels::LBGK<lb::lattices::D3Q15> > hydroVars1(f_original);

            // Calculate density, velocity, equilibrium f.
            lbgk->CalculateDensityVelocityFeq(hydroVars0, 0);

            // Manually set density and velocity and calculate eqm f.
            hydroVars1.density = 1.0;
            hydroVars1.v_x = 0.4;
            hydroVars1.v_y = 0.5;
            hydroVars1.v_z = 0.6;

            lbgk->CalculateFeq(hydroVars1, 1);

            // Calculate expected values.
            distribn_t expectedDensity0 = 12.0; // (sum 1 to 15) / 10
            distribn_t expectedDensity1 = 1.0; // Unchanged

            distribn_t expectedVelocity0[3];
            LbTestsHelper::CalculateVelocity<lb::lattices::D3Q15>(hydroVars0.f, expectedVelocity0);
            distribn_t expectedVelocity1[3] = { 0.4, 0.5, 0.6 };

            distribn_t expectedFEq0[lb::lattices::D3Q15::NUMVECTORS];
            LbTestsHelper::CalculateLBGKEqmF<lb::lattices::D3Q15>(expectedDensity0,
                                                                  expectedVelocity0[0],
                                                                  expectedVelocity0[1],
                                                                  expectedVelocity0[2],
                                                                  expectedFEq0);
            distribn_t expectedFEq1[lb::lattices::D3Q15::NUMVECTORS];
            LbTestsHelper::CalculateLBGKEqmF<lb::lattices::D3Q15>(expectedDensity1,
                                                                  expectedVelocity1[0],
                                                                  expectedVelocity1[1],
                                                                  expectedVelocity1[2],
                                                                  expectedFEq1);

            // Now compare the expected and actual values.
            distribn_t allowedError = 1e-10;

            LbTestsHelper::CompareHydros(expectedDensity0,
                                         expectedVelocity0[0],
                                         expectedVelocity0[1],
                                         expectedVelocity0[2],
                                         expectedFEq0,
                                         "LBGK, case 0",
                                         hydroVars0,
                                         allowedError);
            LbTestsHelper::CompareHydros(expectedDensity1,
                                         expectedVelocity1[0],
                                         expectedVelocity1[1],
                                         expectedVelocity1[2],
                                         expectedFEq1,
                                         "LBGK, case 1",
                                         hydroVars1,
                                         allowedError);

            // Do the collision and test the result.
            lbgk->DoCollide(lbmParams, hydroVars0);
            lbgk->DoCollide(lbmParams, hydroVars1);

            // Get the expected post-collision densities.
            distribn_t expectedPostCollision0[lb::lattices::D3Q15::NUMVECTORS];
            distribn_t expectedPostCollision1[lb::lattices::D3Q15::NUMVECTORS];

            LbTestsHelper::CalculateLBGKCollision<lb::lattices::D3Q15>(f_original,
                                                                       hydroVars0.GetFEq().f,
                                                                       lbmParams->GetOmega(),
                                                                       expectedPostCollision0);

            LbTestsHelper::CalculateLBGKCollision<lb::lattices::D3Q15>(f_original,
                                                                       hydroVars1.GetFEq().f,
                                                                       lbmParams->GetOmega(),
                                                                       expectedPostCollision1);

            // Compare.
            for (unsigned int ii = 0; ii < lb::lattices::D3Q15::NUMVECTORS; ++ii)
            {
              std::stringstream message("Post-collision ");
              message << ii;

              CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(),
                                                   hydroVars0.GetFPostCollision()[ii],
                                                   expectedPostCollision0[ii],
                                                   allowedError);

              CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(),
                                                   hydroVars1.GetFPostCollision()[ii],
                                                   expectedPostCollision1[ii],
                                                   allowedError);
            }
          }

          void TestLBGKNNCalculationsAndCollision()
          {
            /*
             * When testing this streamer, it is important to consider that tau is defined per site.
             * Use two different sets of initial conditions across the domain to check that different
             * shear-rates and relaxation times are computed and stored properly.
             *
             * Using {f_, velocities}setA for odd site indices and {f_, velocities}setB for the even ones
             */
            distribn_t f_setA[lb::lattices::D3Q15::NUMVECTORS], f_setB[lb::lattices::D3Q15::NUMVECTORS];
            distribn_t* f_original;

            for (unsigned int ii = 0; ii < lb::lattices::D3Q15::NUMVECTORS; ++ii)
            {
              f_setA[ii] = ((float) (1 + ii)) / 10.0;
              f_setB[ii] = ((float) (lb::lattices::D3Q15::NUMVECTORS - ii)) / 10.0;
            }

            typedef lb::kernels::LBGKNN<lb::kernels::rheologyModels::CarreauYasudaRheologyModel, lb::lattices::D3Q15> LB_KERNEL;
            lb::kernels::HydroVars<LB_KERNEL> hydroVars0SetA(f_setA), hydroVars1SetA(f_setA);
            lb::kernels::HydroVars<LB_KERNEL> hydroVars0SetB(f_setB), hydroVars1SetB(f_setB);
            lb::kernels::HydroVars<LB_KERNEL> *hydroVars0 = NULL, *hydroVars1 = NULL;

            distribn_t velocitiesSetA[] = { 0.4, 0.5, 0.6 };
            distribn_t velocitiesSetB[] = { -0.4, -0.5, -0.6 };
            distribn_t *velocities;

            distribn_t numTolerance = 1e-10;

            for (site_t site_index = 0; site_index < numSites; site_index++)
            {
              /*
               * Test part 1: Equilibrium function, density, and velocity are computed
               * identically to the standard LBGK. Local relaxation times are implicitely
               * computed by CalculateDensityVelocityFeq
               */

              /*
               * Case 0: test the kernel function for calculating density, velocity
               * and f_eq.
               * Case 1: test the function that uses a given density and velocity, and
               * calculates f_eq.
               */
              if (site_index % 2)
              {
                f_original = f_setA;
                hydroVars0 = &hydroVars0SetA;
                hydroVars1 = &hydroVars1SetA;
                velocities = velocitiesSetA;
              }
              else
              {
                f_original = f_setB;
                hydroVars0 = &hydroVars0SetB;
                hydroVars1 = &hydroVars1SetB;
                velocities = velocitiesSetB;
              }

              // Calculate density, velocity, equilibrium f.
              lbgknn0->CalculateDensityVelocityFeq(*hydroVars0, site_index);

              // Manually set density and velocity and calculate eqm f.
              hydroVars1->density = 1.0;
              hydroVars1->v_x = velocities[0];
              hydroVars1->v_y = velocities[1];
              hydroVars1->v_z = velocities[2];

              lbgknn1->CalculateFeq(*hydroVars1, site_index);

              // Calculate expected values.
              distribn_t expectedDensity0 = 12.0; // (sum 1 to 15) / 10
              distribn_t expectedDensity1 = 1.0; // Unchanged

              distribn_t expectedVelocity0[3];
              LbTestsHelper::CalculateVelocity<lb::lattices::D3Q15>(hydroVars0->f, expectedVelocity0);
              distribn_t *expectedVelocity1 = velocities;

              distribn_t expectedFEq0[lb::lattices::D3Q15::NUMVECTORS];
              LbTestsHelper::CalculateLBGKEqmF<lb::lattices::D3Q15>(expectedDensity0,
                                                                    expectedVelocity0[0],
                                                                    expectedVelocity0[1],
                                                                    expectedVelocity0[2],
                                                                    expectedFEq0);
              distribn_t expectedFEq1[lb::lattices::D3Q15::NUMVECTORS];
              LbTestsHelper::CalculateLBGKEqmF<lb::lattices::D3Q15>(expectedDensity1,
                                                                    expectedVelocity1[0],
                                                                    expectedVelocity1[1],
                                                                    expectedVelocity1[2],
                                                                    expectedFEq1);

              // Now compare the expected and actual values.
              LbTestsHelper::CompareHydros(expectedDensity0,
                                           expectedVelocity0[0],
                                           expectedVelocity0[1],
                                           expectedVelocity0[2],
                                           expectedFEq0,
                                           "LBGKNN, case 0",
                                           *hydroVars0,
                                           numTolerance);
              LbTestsHelper::CompareHydros(expectedDensity1,
                                           expectedVelocity1[0],
                                           expectedVelocity1[1],
                                           expectedVelocity1[2],
                                           expectedFEq1,
                                           "LBGKNN, case 1",
                                           *hydroVars1,
                                           numTolerance);

              /*
               * Test part 2: Test that the array containing the local relaxation
               * times has the right length and test against some hardcoded values.
               * Correctness of the relaxation time calculator is tested in RheologyModelTest.h
               */

              /*
               * A second call to the Calculate* functions will make sure that the newly computed
               * tau is used in DoCollide as opposite to the default Newtonian tau used during the
               * first time step.
               */
              lbgknn0->CalculateDensityVelocityFeq(*hydroVars0, site_index);
              lbgknn1->CalculateFeq(*hydroVars1, site_index);

              distribn_t computedTau0 = hydroVars0->tau;
              CPPUNIT_ASSERT_EQUAL_MESSAGE("Tau array size ", numSites, (site_t) lbgknn0->GetTauValues().size());

              distribn_t expectedTau0 = site_index % 2 ?
                0.50009134451 :
                0.50009285237;

              std::stringstream message;
              message << "Tau array [" << site_index << "] for dataset 0";
              CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(), expectedTau0, computedTau0, numTolerance);

              distribn_t computedTau1 = hydroVars1->tau;
              CPPUNIT_ASSERT_EQUAL_MESSAGE("Tau array size ", numSites, (site_t) lbgknn1->GetTauValues().size());

              distribn_t expectedTau1 = site_index % 2 ?
                0.50009013551 :
                0.50009021207;

              message.str("");
              message << "Tau array [" << site_index << "] for dataset 1";
              CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(), expectedTau1, computedTau1, numTolerance);

              /*
               * Test part 3: Collision depends on the local relaxation time
               */
              // Do the collision and test the result.
              lbgknn0->DoCollide(lbmParams, *hydroVars0);
              lbgknn1->DoCollide(lbmParams, *hydroVars1);

              // Get the expected post-collision densities.
              distribn_t expectedPostCollision0[lb::lattices::D3Q15::NUMVECTORS];
              distribn_t expectedPostCollision1[lb::lattices::D3Q15::NUMVECTORS];

              distribn_t localOmega0 = -1.0 / computedTau0;
              distribn_t localOmega1 = -1.0 / computedTau1;

              LbTestsHelper::CalculateLBGKCollision<lb::lattices::D3Q15>(f_original,
                                                                         hydroVars0->GetFEq().f,
                                                                         localOmega0,
                                                                         expectedPostCollision0);

              LbTestsHelper::CalculateLBGKCollision<lb::lattices::D3Q15>(f_original,
                                                                         hydroVars1->GetFEq().f,
                                                                         localOmega1,
                                                                         expectedPostCollision1);

              // Compare.
              for (unsigned int ii = 0; ii < lb::lattices::D3Q15::NUMVECTORS; ++ii)
              {
                std::stringstream message;
                message << "Post-collision: site " << site_index << " direction " << ii;

                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(),
                                                     hydroVars0->GetFPostCollision()[ii],
                                                     expectedPostCollision0[ii],
                                                     numTolerance);

                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(),
                                                     hydroVars1->GetFPostCollision()[ii],
                                                     expectedPostCollision1[ii],
                                                     numTolerance);
              }
            }
          }

          void TestMRTConstantRelaxationTimeEqualsLBGK()
          {
            /*
             *  Simulate LBGK by relaxing all the MRT modes to equilibrium with the same time constant.
             */
            std::vector<distribn_t> relaxationParameters;
            distribn_t oneOverTau = 1.0 / lbmParams->GetTau();
            relaxationParameters.resize(lb::kernels::momentBasis::DHumieresD3Q15MRTBasis::NUM_KINETIC_MOMENTS,
                                        oneOverTau);
            mrtLbgkEquivalentKernel->SetMrtRelaxationParameters(relaxationParameters);

            // Initialise the original f distribution to something asymmetric.
            distribn_t f_original[lb::lattices::D3Q15::NUMVECTORS];
            LbTestsHelper::InitialiseAnisotropicTestData<lb::lattices::D3Q15>(0, f_original);
            lb::kernels::HydroVars<lb::kernels::MRT<lb::kernels::momentBasis::DHumieresD3Q15MRTBasis> > hydroVars0(f_original);

            // Calculate density, velocity, equilibrium f.
            mrtLbgkEquivalentKernel->CalculateDensityVelocityFeq(hydroVars0, 0);

            // Calculate expected values for the configuration of the MRT kernel equivalent to LBGK.
            distribn_t expectedDensity0;
            distribn_t expectedVelocity0[3];
            distribn_t expectedFEq0[lb::lattices::D3Q15::NUMVECTORS];
            LbTestsHelper::CalculateRhoVelocity<lb::lattices::D3Q15>(hydroVars0.f, expectedDensity0, expectedVelocity0);
            LbTestsHelper::CalculateLBGKEqmF<lb::lattices::D3Q15>(expectedDensity0,
                                                                  expectedVelocity0[0],
                                                                  expectedVelocity0[1],
                                                                  expectedVelocity0[2],
                                                                  expectedFEq0);

            // Now compare the expected and actual values.
            distribn_t allowedError = 1e-10;
            LbTestsHelper::CompareHydros(expectedDensity0,
                                         expectedVelocity0[0],
                                         expectedVelocity0[1],
                                         expectedVelocity0[2],
                                         expectedFEq0,
                                         "MRT against LBGK",
                                         hydroVars0,
                                         allowedError);

            // Do the MRT collision.
            mrtLbgkEquivalentKernel->DoCollide(lbmParams, hydroVars0);

            // Get the expected post-collision velocity distributions with LBGK.
            distribn_t expectedPostCollision0[lb::lattices::D3Q15::NUMVECTORS];
            LbTestsHelper::CalculateLBGKCollision<lb::lattices::D3Q15>(f_original,
                                                                       hydroVars0.GetFEq().f,
                                                                       lbmParams->GetOmega(),
                                                                       expectedPostCollision0);

            // Compare.
            for (unsigned int ii = 0; ii < lb::lattices::D3Q15::NUMVECTORS; ++ii)
            {
              std::stringstream message;
              message << "Post-collision " << ii;

              CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(),
                                                   hydroVars0.GetFPostCollision()[ii],
                                                   expectedPostCollision0[ii],
                                                   allowedError);
            }
          }

          void TestD3Q19MRTConstantRelaxationTimeEqualsLBGK()
          {
            /*
             *  Simulate LBGK by relaxing all the MRT modes to equilibrium with the same time constant.
             */
            std::vector<distribn_t> relaxationParameters;
            distribn_t oneOverTau = 1.0 / lbmParams->GetTau();
            relaxationParameters.resize(lb::kernels::momentBasis::DHumieresD3Q19MRTBasis::NUM_KINETIC_MOMENTS,
                                        oneOverTau);
            mrtLbgkEquivalentKernel19->SetMrtRelaxationParameters(relaxationParameters);

            // Initialise the original f distribution to something asymmetric.
            distribn_t f_original[lb::lattices::D3Q19::NUMVECTORS];
            LbTestsHelper::InitialiseAnisotropicTestData<lb::lattices::D3Q19>(0, f_original);
            lb::kernels::HydroVars<lb::kernels::MRT<lb::kernels::momentBasis::DHumieresD3Q19MRTBasis> > hydroVars0(f_original);

            // Calculate density, velocity, equilibrium f.
            mrtLbgkEquivalentKernel19->CalculateDensityVelocityFeq(hydroVars0, 0);

            // Calculate expected values for the configuration of the MRT kernel equivalent to LBGK.
            distribn_t expectedDensity0;
            distribn_t expectedVelocity0[3];
            distribn_t expectedFEq0[lb::lattices::D3Q19::NUMVECTORS];
            LbTestsHelper::CalculateRhoVelocity<lb::lattices::D3Q19>(hydroVars0.f, expectedDensity0, expectedVelocity0);
            LbTestsHelper::CalculateLBGKEqmF<lb::lattices::D3Q19>(expectedDensity0,
                                                                  expectedVelocity0[0],
                                                                  expectedVelocity0[1],
                                                                  expectedVelocity0[2],
                                                                  expectedFEq0);

            // Now compare the expected and actual values.
            distribn_t allowedError = 1e-10;
            LbTestsHelper::CompareHydros(expectedDensity0,
                                         expectedVelocity0[0],
                                         expectedVelocity0[1],
                                         expectedVelocity0[2],
                                         expectedFEq0,
                                         "MRT against LBGK",
                                         hydroVars0,
                                         allowedError);

            // Do the MRT collision.
            mrtLbgkEquivalentKernel19->DoCollide(lbmParams, hydroVars0);

            // Get the expected post-collision velocity distributions with LBGK.
            distribn_t expectedPostCollision0[lb::lattices::D3Q19::NUMVECTORS];
            LbTestsHelper::CalculateLBGKCollision<lb::lattices::D3Q19>(f_original,
                                                                       hydroVars0.GetFEq().f,
                                                                       lbmParams->GetOmega(),
                                                                       expectedPostCollision0);

            // Compare.
            for (unsigned int ii = 0; ii < lb::lattices::D3Q19::NUMVECTORS; ++ii)
            {
              std::stringstream message;
              message << "Post-collision " << ii;

              CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(),
                                                   hydroVars0.GetFPostCollision()[ii],
                                                   expectedPostCollision0[ii],
                                                   allowedError);
            }
          }

        private:
          lb::kernels::EntropicAnsumali<lb::lattices::D3Q15>* entropic;
          lb::kernels::LBGK<lb::lattices::D3Q15>* lbgk;
          lb::kernels::LBGKNN<lb::kernels::rheologyModels::CarreauYasudaRheologyModel, lb::lattices::D3Q15> *lbgknn0,
              *lbgknn1;
          lb::kernels::MRT<lb::kernels::momentBasis::DHumieresD3Q15MRTBasis>* mrtLbgkEquivalentKernel;
          lb::kernels::MRT<lb::kernels::momentBasis::DHumieresD3Q19MRTBasis>* mrtLbgkEquivalentKernel19;
      };
      CPPUNIT_TEST_SUITE_REGISTRATION (KernelTests);
    }
  }
}

#endif /* HEMELB_UNITTESTS_LBTESTS_KERNELTESTS_H */
