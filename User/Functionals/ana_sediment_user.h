      SUBROUTINE ana_sediment (ng, tile, model)
!
!! svn $Id$
!!======================================================================
!! Copyright (c) 2002-2007 The ROMS/TOMS Group                         !
!!   Licensed under a MIT/X style license                              !
!!   See License_ROMS.txt                                              !
!!                                                                     !
!=======================================================================
!                                                                      !
!  This routine sets initial conditions for  sedimen t tracer fields   !
!  concentrations  (kg/m3) using analytical expressions for sediment   !
!  and/or bottom boundary layer configurations. It also sets initial   !
!  bed conditions in each sediment layer.                              !
!                                                                      !
!=======================================================================
!
      USE mod_param
      USE mod_grid
      USE mod_ocean
!
! Imported variable declarations.
!
      integer, intent(in) :: ng, tile, model

#include "tile.h"
!
      CALL ana_sediment_tile (ng, model, Istr, Iend, Jstr, Jend,        &
     &                        LBi, UBi, LBj, UBj,                       &
     &                        GRID(ng) % pm,                            &
     &                        GRID(ng) % pn,                            &
     &                        GRID(ng) % xr,                            &
     &                        GRID(ng) % yr,                            &
#if defined BBL_MODEL && (defined MB_BBL || defined SSW_BBL)
     &                        OCEAN(ng) % rho,                          &
#endif
#ifdef SEDIMENT
     &                        OCEAN(ng) % t,                            &
     &                        OCEAN(ng) % bed,                          &
     &                        OCEAN(ng) % bed_frac,                     &
     &                        OCEAN(ng) % bed_mass,                     &
#endif
     &                        OCEAN(ng) % bottom)
      RETURN
      END SUBROUTINE ana_sediment
!
!***********************************************************************
      SUBROUTINE ana_sediment_tile (ng, model, Istr, Iend, Jstr, Jend,  &
     &                              LBi, UBi, LBj, UBj,                 &
     &                              pm, pn,                             &
     &                              xr, yr,                             &
#if defined BBL_MODEL && (defined MB_BBL || defined SSW_BBL)
     &                              rho,                                &
#endif
#ifdef SEDIMENT
     &                              t,                                  &
     &                              bed, bed_frac, bed_mass,            &
#endif
     &                              bottom)
!***********************************************************************
!
      USE mod_param
      USE mod_scalars
      USE mod_sediment
!
#if defined EW_PERIODIC || defined NS_PERIODIC
      USE exchange_3d_mod, ONLY : exchange_r3d_tile
#endif
#ifdef DISTRIBUTE
      USE mp_exchange_mod, ONLY : mp_exchange3d, mp_exchange4d
#endif
!
!  Imported variable declarations.
!
      integer, intent(in) :: ng, model, Iend, Istr, Jend, Jstr
      integer, intent(in) :: LBi, UBi, LBj, UBj
!
#ifdef ASSUMED_SHAPE
      real(r8), intent(in) :: pm(LBi:,LBj:)
      real(r8), intent(in) :: pn(LBi:,LBj:)
      real(r8), intent(in) :: xr(LBi:,LBj:)
      real(r8), intent(in) :: yr(LBi:,LBj:)
# if defined BBL_MODEL && (defined MB_BBL || defined SSW_BBL)
      real(r8), intent(in) :: rho(LBi:,LBj:,:)
# endif
# ifdef SEDIMENT
      real(r8), intent(inout) :: t(LBi:,LBj:,:,:,:)
      real(r8), intent(out) :: bed(LBi:,LBj:,:,:)
      real(r8), intent(out) :: bed_frac(LBi:,LBj:,:,:)
      real(r8), intent(out) :: bed_mass(LBi:,LBj:,:,:,:)
# endif
      real(r8), intent(inout) :: bottom(LBi:,LBj:,:)
#else
      real(r8), intent(in) :: pm(LBi:UBi,LBj:UBj)
      real(r8), intent(in) :: pn(LBi:UBi,LBj:UBj)
      real(r8), intent(in) :: xr(LBi:UBi,LBj:UBj)
      real(r8), intent(in) :: yr(LBi:UBi,LBj:UBj)
# if defined BBL_MODEL && (defined MB_BBL || defined SSW_BBL)
      real(r8), intent(in) :: rho(LBi:,LBj:,:)
# endif
# ifdef SEDIMENT
      real(r8), intent(inout) :: t(LBi:UBi,LBj:UBj,N(ng),3,NT(ng))
      real(r8), intent(out) :: bed(LBi:UBi,LBj:UBj,Nbed,MBEDP)
      real(r8), intent(out) :: bed_frac(LBi:UBi,LBj:UBj,Nbed,NST)
      real(r8), intent(out) :: bed_mass(LBi:UBi,LBj:UBj,Nbed,2,NST)
# endif
      real(r8), intent(inout) :: bottom(LBi:UBi,LBj:UBj,MBOTP)
#endif
!
!  Local variable declarations.
!
#ifdef DISTRIBUTE
# ifdef EW_PERIODIC
      logical :: EWperiodic=.TRUE.
# else
      logical :: EWperiodic=.FALSE.
# endif
# ifdef NS_PERIODIC
      logical :: NSperiodic=.TRUE.
# else
      logical :: NSperiodic=.FALSE.
# endif
#endif

      integer :: IstrR, IendR, JstrR, JendR, IstrU, JstrV
      integer :: i, ised, j, k

      real(r8) :: cff1, cff2, cff3, cff4

#include "set_bounds.h"

#if defined BBL_MODEL && !defined SEDIMENT
!
!-----------------------------------------------------------------------
!  If only bottom boundary layer and not sediment model, set bottom
!  sediment grain diameter (m) and density (kg/m3).
!-----------------------------------------------------------------------
!
# if defined MY_APPLICATION
      DO j=JstrR,JendR
        DO i=IstrR,IendR
          bottom(i,j,isd50)=???
          bottom(i,j,idens)=???
        END DO
      END DO
# else
      ana_sediment_user.h: no values provided for bottom(:,:,isd50) and
                                                  bottom(:,:,idens).
# endif

# if defined MB_BBL || defined SSW_BBL
!
!-----------------------------------------------------------------------
!  If only Blass bottom boundary layer and not sediment model, set
!  set critical (threshold) bedload stress (m2/s2).
!-----------------------------------------------------------------------
!
#  if defined MY_APPLICATION
      DO j=JstrR,JendR
        DO i=IstrR,IendR
          bottom(i,j,itauc)=???
        END DO
      END DO
#  else
      ana_sediment_user.h: no values provided for bottom(:,:,itauc).
#  endif
# endif

# if defined MB_BBL
!
!-----------------------------------------------------------------------
!  If only Blass bottom boundary layer and not sediment model, set
!  sediiment settling velocity (m/s).
!-----------------------------------------------------------------------
!
#  if defined MY_APPLICATION
      DO j=JstrR,JendR
        DO i=IstrR,IendR
          bottom(i,j,iwsed)=???
        END DO
      END DO
#  else
      ana_sediment_user.h: no values provided for bottom(:,:,iwsed).
#  endif
# endif

#endif

#ifdef SEDIMENT
!
!-----------------------------------------------------------------------
!  Initial sediment concentrations in the water column.
!-----------------------------------------------------------------------
!
      DO ised=1,NST
        DO k=1,N(ng)
          DO j=JstrR,JendR
            DO i=IstrR,IendR
              t(i,j,k,1,idsed(ised))=Csed(ised,ng)
              t(i,j,k,2,idsed(ised))=t(i,j,k,1,idsed(ised))
            END DO
          END DO
        END DO
      END DO
!
!-----------------------------------------------------------------------
!  Initial sediment bed layer properties.
!-----------------------------------------------------------------------
!
# if defined MY_APPLICATION
      DO k=1,Nbed
        DO j=JstrR,JendR
          DO i=IstrR,IendR
             bed(i,j,k,iaged)=???
             bed(i,j,k,ithck)=???
             bed(i,j,k,iporo)=???
             DO ised=1,NST
               bed_frac(i,j,k,ised)=1.0_r8/FLOAT(NST)
             END DO
!
!  Calculate mass so it is consistent with density, thickness, and
!  porosity.
!
             DO ised=1,NST
               bed_mass(i,j,k,1,ised)=???
               bed_mass(i,j,k,2,ised)=bed_mass(i,j,k,1,ised)
             END DO
          END DO
        END DO
      END DO
!
!  Set exposed sediment layer properties.
!
      DO j=JstrR,JendR
        DO i=IstrR,IendR
          cff1=1.0_r8
          cff2=1.0_r8
          cff3=1.0_r8
          cff4=1.0_r8
          DO ised=1,NST
            cff1=cff1*Sd50(ised,ng)**bed_frac(i,j,1,ised)
            cff2=cff2*Srho(ised,ng)**bed_frac(i,j,1,ised)
            cff3=cff3*wsed(ised,ng)**bed_frac(i,j,1,ised)
            cff4=cff4*tau_ce(ised,ng)**bed_frac(i,j,1,ised)
          END DO
          bottom(i,j,isd50)=cff1
          bottom(i,j,idens)=cff2
          bottom(i,j,iwsed)=cff3
          bottom(i,j,itauc)=cff4
          bottom(i,j,irlen)=0.10_r8
          bottom(i,j,irhgt)=0.01_r8
          bottom(i,j,izdef)=Zob(ng)
#  ifdef SED_BIODIFF
          bottom(i,j,idoff)=0.0_r8
          bottom(i,j,idslp)=0.0_r8
          bottom(i,j,idtim)=0.0_r8
          bottom(i,j,idbmx)=0.0_r8
          bottom(i,j,idbmm)=0.0_r8
          bottom(i,j,idbzs)=0.0_r8
          bottom(i,j,idbzm)=0.0_r8
          bottom(i,j,idbzp)=0.0_r8
#  endif
        END DO
      END DO
# else
      ana_sediment_user.h: no values provided for bed, bed_mass, bottom.
# endif

#endif

#if defined EW_PERIODIC || defined NS_PERIODIC || defined DISTRIBUTE
!
!-----------------------------------------------------------------------
!  Exchange boundary data.
!-----------------------------------------------------------------------
!
# if defined EW_PERIODIC || defined NS_PERIODIC
#  ifdef SEDIMENT
      DO ised=1,NST
        CALL exchange_r3d_tile (ng, Istr, Iend, Jstr, Jend,             &
     &                          LBi, UBi, LBj, UBj, 1, Nbed,            &
     &                          bed_frac(:,:,:,ised))
        CALL exchange_r3d_tile (ng, Istr, Iend, Jstr, Jend,             &
     &                          LBi, UBi, LBj, UBj, 1, Nbed,            &
     &                          bed_mass(:,:,:,1,ised))
      END DO
      DO ised=1,MBEDP
        CALL exchange_r3d_tile (ng, Istr, Iend, Jstr, Jend,             &
     &                          LBi, UBi, LBj, UBj, 1, Nbed,            &
     &                          bed(:,:,:,ised))
      END DO
#  endif
      CALL exchange_r3d_tile (ng, Istr, Iend, Jstr, Jend,               &
     &                        LBi, UBi, LBj, UBj, 1, MBOTP,             &
     &                        bottom)
# endif
# ifdef DISTRIBUTE
#  ifdef SEDIMENT
      CALL mp_exchange4d (ng, model, 2, Istr, Iend, Jstr, Jend,         &
     &                    LBi, UBi, LBj, UBj, 1, Nbed, 1, NST,          &
     &                    NghostPoints, EWperiodic, NSperiodic,         &
     &                    bed_frac,                                     &
     &                    bed_mass(:,:,:,1,:))
      CALL mp_exchange4d (ng, model, 1, Istr, Iend, Jstr, Jend,         &
     &                    LBi, UBi, LBj, UBj, 1, Nbed, 1, MBEDP,        &
     &                    NghostPoints, EWperiodic, NSperiodic,         &
     &                    bed)
#  endif
      CALL mp_exchange3d (ng, model, 1, Istr, Iend, Jstr, Jend,         &
     &                    LBi, UBi, LBj, UBj, 1, MBOTP,                 &
     &                    NghostPoints, EWperiodic, NSperiodic,         &
     &                    bottom)
# endif
#endif

      RETURN
      END SUBROUTINE ana_sediment_tile