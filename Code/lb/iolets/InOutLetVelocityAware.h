//
// Copyright (C) University College London, 2007-2012, all rights reserved.
//
// This file is part of HemeLB and is CONFIDENTIAL. You may not work
// with, install, use, duplicate, modify, redistribute or share this
// file, or any part thereof, other than as allowed by any agreement
// specifically made by you with University College London.
//

#ifndef HEMELB_LB_BOUNDARIES_IOLETS_INOUTLETVELOCITYAWARE_H
#define HEMELB_LB_BOUNDARIES_IOLETS_INOUTLETVELOCITYAWARE_H

#include "lb/iolets/InOutLetMultiscale.h"
#include "geometry/LatticeData.h"
#include "geometry/neighbouring/NeighbouringDataManager.h"
#include "log/Logger.h"
#include "lb/MacroscopicPropertyCache.h"

namespace hemelb
{
  namespace geometry
  {
    class LatticeData;
    namespace neighbouring
    {
      class NeighbouringDataManager;
    }
  }

  namespace lb
  {
    class MacroscopicPropertyCache;

    namespace iolets
    {

      /***
       * An inlet/outlet whose density is obtained from the multiscale intercommunicator
       * We envisage communication of velocity information outwards to other processes
       * the velocity SharedValue is a place-holder for this.
       * We set 0.1 as an arbitrary value for the output velocity for now.
       * We do not yet have an understanding of the necessary physics, or the necessary computational infrastructure,
       * for building boundary conditions which have access to their nearby velocity field.
       * The min and max pressure SharedValues are placeholders for information needed by the steering and visualisation code.
       * The steering and visualisation code requires minimum and maximum pressure values.
       */
      class InOutLetVelocityAware : public InOutLetMultiscale
      {
        public:
          InOutLetVelocityAware();
          /**
           * Copy constructor
           */
          InOutLetVelocityAware(const InOutLetVelocityAware &other);
          virtual ~InOutLetVelocityAware();

          void
              InitialiseNeighbouringSites(
                                          geometry::neighbouring::NeighbouringDataManager *manager,
                                          geometry::LatticeData * latDat,
                                          hemelb::lb::MacroscopicPropertyCache* globalPropertyCache,
                                          std::vector<site_t> invBList);

          PhysicalVelocity_deprecated GetVelocity() const;
          virtual InOutLet* Clone() const;

        protected:
          geometry::neighbouring::NeighbouringDataManager *NDM;
          hemelb::lb::MacroscopicPropertyCache* propertyCache;
          std::vector<site_t> sitesWhichNeighbourThisBoundary;
          geometry::LatticeData * latticeData;
      };
    }
  }
}

#endif /* HEMELB_LB_BOUNDARIES_IOLETS_INOUTLETVELOCITYAWARE_H */