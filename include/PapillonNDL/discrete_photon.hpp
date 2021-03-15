/*
 * Copyright 2021, Hunter Belanger
 *
 * hunter.belanger@gmail.com
 *
 * Ce logiciel est régi par la licence CeCILL soumise au droit français et
 * respectant les principes de diffusion des logiciels libres. Vous pouvez
 * utiliser, modifier et/ou redistribuer ce programme sous les conditions
 * de la licence CeCILL telle que diffusée par le CEA, le CNRS et l'INRIA
 * sur le site "http://www.cecill.info".
 *
 * En contrepartie de l'accessibilité au code source et des droits de copie,
 * de modification et de redistribution accordés par cette licence, il n'est
 * offert aux utilisateurs qu'une garantie limitée.  Pour les mêmes raisons,
 * seule une responsabilité restreinte pèse sur l'auteur du programme,  le
 * titulaire des droits patrimoniaux et les concédants successifs.
 *
 * A cet égard  l'attention de l'utilisateur est attirée sur les risques
 * associés au chargement,  à l'utilisation,  à la modification et/ou au
 * développement et à la reproduction du logiciel par l'utilisateur étant
 * donné sa spécificité de logiciel libre, qui peut le rendre complexe à
 * manipuler et qui le réserve donc à des développeurs et des professionnels
 * avertis possédant  des  connaissances  informatiques approfondies.  Les
 * utilisateurs sont donc invités à charger  et  tester  l'adéquation  du
 * logiciel à leurs besoins dans des conditions permettant d'assurer la
 * sécurité de leurs systèmes et ou de leurs données et, plus généralement,
 * à l'utiliser et l'exploiter dans les mêmes conditions de sécurité.
 *
 * Le fait que vous puissiez accéder à cet en-tête signifie que vous avez
 * pris connaissance de la licence CeCILL, et que vous en avez accepté les
 * termes.
 *
 * */
#ifndef PAPILLON_NDL_DISCRETE_PHOTON_H
#define PAPILLON_NDL_DISCRETE_PHOTON_H

/**
 * @file
 * @author Hunter Belanger
 */

#include <PapillonNDL/ace.hpp>
#include <PapillonNDL/energy_law.hpp>

namespace pndl {

/**
 * @brief Energy distribution for discrete photons.
 */
class DiscretePhoton : public EnergyLaw {
  public:
    DiscretePhoton(const ACE& ace, size_t i): lp(), A(), Eg() {
      lp = ace.xss<int>(i);
      Eg = ace.xss(i+1);
      A = ace.awr();
    }
    ~DiscretePhoton() = default;

    double sample_energy(double E_in, std::function<double()> /*rng*/) const override final {
      if((lp == 0) || (lp == 1)) return Eg;
      return Eg + (A / (A+1.))*E_in;
    }

    /**
     * @brief Returns the flay indicating whether the photon is a primary or secondary.
     *        A secondary photon corresponds with 0 and 1, while a secondary photon has
     *        a value of 2.
     */
    int primary_indicator() const {return lp;}

    /**
     * @brief Returns the energy argument for the distribution. If it is a primary photon,
     *        this is the outgoing energy, and for a secondary neutron this is the binding
     *        energy.
     */
    double photon_energy() const {return Eg;}

  private:
    int lp;
    double A;
    double Eg;
};

}

#endif