/*
 * Papillon Nuclear Data Library
 * Copyright 2021-2023, Hunter Belanger
 *
 * hunter.belanger@gmail.com
 *
 * This file is part of the Papillon Nuclear Data Library (PapillonNDL).
 *
 * PapillonNDL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PapillonNDL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PapillonNDL. If not, see <https://www.gnu.org/licenses/>.
 *
 * */
#include <PapillonNDL/tabular_energy_angle.hpp>

namespace pndl {

TabularEnergyAngle::TabularEnergyAngle(const ACE& ace, std::size_t i,
                                       std::size_t JED)
    : incoming_energy_(), tables_() {
  // Get number of interpolation points
  uint32_t NR = ace.xss<uint32_t>(i);
  // Get number of energy points
  uint32_t NE = ace.xss<uint32_t>(i + 1 + 2 * NR);

  // Breakpoints and interpolations are not read, as linear-linear
  // interpolation is always used between incoming energies.

  // Read incoming energies
  incoming_energy_ = ace.xss(i + 2 + 2 * NR, NE);

  if (!std::is_sorted(incoming_energy_.begin(), incoming_energy_.end())) {
    std::string mssg =
        "Incoming energy grid is not sorted. Index to TabularEnergyAngle in "
        "XSS block is " +
        std::to_string(i) + ".";
    throw PNDLException(mssg);
  }

  // Read outgoing energy tables
  for (uint32_t j = 0; j < NE; j++) {
    uint32_t loc = static_cast<uint32_t>(ace.DLW()) +
                   ace.xss<uint32_t>(i + 2 + 2 * NR + NE + j) - 1;
    try {
      tables_.emplace_back(ace, loc, JED);
    } catch (PNDLException& error) {
      std::string mssg = "Could not create EnergyAngleTable for the " +
                         std::to_string(j) + "th incoming energy " +
                         std::to_string(incoming_energy_[j]) +
                         " MeV. Index of TabularEnergyAngle in XSS block is " +
                         std::to_string(i) + ".";
      error.add_to_exception(mssg);
      throw error;
    }
  }
}

TabularEnergyAngle::TabularEnergyAngle(
    const std::vector<double>& incoming_energy,
    const std::vector<EnergyAngleTable>& tables)
    : incoming_energy_(incoming_energy), tables_(tables) {
  if (!std::is_sorted(incoming_energy_.begin(), incoming_energy_.end())) {
    std::string mssg = "Incoming energy grid is not sorted.";
    throw PNDLException(mssg);
  }

  if (incoming_energy_.size() != tables_.size()) {
    std::string mssg =
        "Must have the same number of points in the incoming energy grid as "
        "there are KalbachTables for the outgoing energy and angle.";
    throw PNDLException(mssg);
  }
}

AngleEnergyPacket TabularEnergyAngle::sample_angle_energy(
    double E_in, const std::function<double()>& rng) const {
  // Determine the index of the bounding tabulated incoming energies
  std::size_t l;
  double f;  // Interpolation factor
  auto in_E_it =
      std::lower_bound(incoming_energy_.begin(), incoming_energy_.end(), E_in);
  if (in_E_it == incoming_energy_.begin()) {
    l = 0;
    f = 0.;
  } else if (in_E_it == incoming_energy_.end()) {
    l = incoming_energy_.size() - 2;
    f = 1.;
  } else {
    l = static_cast<std::size_t>(
        std::distance(incoming_energy_.begin(), in_E_it) - 1);
    f = (E_in - incoming_energy_[l]) /
        (incoming_energy_[l + 1] - incoming_energy_[l]);
  }

  // Sample outgoing energy, and get R and A for mu
  double E_i_1 = tables_[l].min_energy();
  double E_i_M = tables_[l].max_energy();
  double E_i_1_1 = tables_[l + 1].min_energy();
  double E_i_1_M = tables_[l + 1].max_energy();
  double Emin = E_i_1 + f * (E_i_1_1 - E_i_1);
  double Emax = E_i_M + f * (E_i_1_M - E_i_M);

  AngleEnergyPacket tmp{0., 0.};
  double E_hat = E_in;
  double E_l_1, E_l_M;
  if (rng() > f) {
    tmp = tables_[l].sample_angle_energy(rng);
    E_hat = tmp.energy;
    E_l_1 = E_i_1;
    E_l_M = E_i_M;
  } else {
    tmp = tables_[l + 1].sample_angle_energy(rng);
    E_hat = tmp.energy;
    E_l_1 = E_i_1_1;
    E_l_M = E_i_1_M;
  }

  double E_out = Emin + ((E_hat - E_l_1) / (E_l_M - E_l_1)) * (Emax - Emin);
  double mu = tmp.cosine_angle;

  // mu has already been verified by the EnergyAngleTable to be in interval
  // [-1,1]

  return {mu, E_out};
}

std::optional<double> TabularEnergyAngle::angle_pdf(double E_in,
                                                    double mu) const {
  // Determine the index of the bounding tabulated incoming energies
  std::size_t l;
  double f;  // Interpolation factor
  auto in_E_it =
      std::lower_bound(incoming_energy_.begin(), incoming_energy_.end(), E_in);
  if (in_E_it == incoming_energy_.begin()) {
    l = 0;
    f = 0.;
  } else if (in_E_it == incoming_energy_.end()) {
    l = incoming_energy_.size() - 2;
    f = 1.;
  } else {
    l = static_cast<std::size_t>(
        std::distance(incoming_energy_.begin(), in_E_it) - 1);
    f = (E_in - incoming_energy_[l]) /
        (incoming_energy_[l + 1] - incoming_energy_[l]);
  }

  double pdf_out = 0.;

  // Do the l table portion
  pdf_out += (1. - f) * tables_[l].angle_pdf(mu);

  // Do the l+1 table portion
  pdf_out += f * tables_[l + 1].angle_pdf(mu);

  return pdf_out;
}

std::optional<double> TabularEnergyAngle::pdf(double E_in, double mu,
                                              double E_out) const {
  // Determine the index of the bounding tabulated incoming energies
  std::size_t l;
  double f;  // Interpolation factor
  auto in_E_it =
      std::lower_bound(incoming_energy_.begin(), incoming_energy_.end(), E_in);
  if (in_E_it == incoming_energy_.begin()) {
    l = 0;
    f = 0.;
  } else if (in_E_it == incoming_energy_.end()) {
    l = incoming_energy_.size() - 2;
    f = 1.;
  } else {
    l = static_cast<std::size_t>(
        std::distance(incoming_energy_.begin(), in_E_it) - 1);
    f = (E_in - incoming_energy_[l]) /
        (incoming_energy_[l + 1] - incoming_energy_[l]);
  }

  // Sample outgoing energy, and get R and A for mu
  double E_i_1 = tables_[l].min_energy();
  double E_i_M = tables_[l].max_energy();
  double E_i_1_1 = tables_[l + 1].min_energy();
  double E_i_1_M = tables_[l + 1].max_energy();
  double Emin = E_i_1 + f * (E_i_1_1 - E_i_1);
  double Emax = E_i_M + f * (E_i_1_M - E_i_M);

  double E_hat = E_in;
  double E_l_1 = 0., E_l_M = 0.;

  double pdf_out = 0.;

  // Do the l table portion
  E_l_1 = E_i_1;
  E_l_M = E_i_M;
  E_hat = ((E_out - Emin) / (Emax - Emin)) * (E_l_M - E_l_1) + E_l_1;
  pdf_out += (1. - f) * tables_[l].pdf(mu, E_hat);

  // Do the l+1 table portion
  E_l_1 = E_i_1_1;
  E_l_M = E_i_1_M;
  E_hat = ((E_out - Emin) / (Emax - Emin)) * (E_l_M - E_l_1) + E_l_1;
  pdf_out += f * tables_[l + 1].pdf(mu, E_hat);

  return pdf_out;
}

}  // namespace pndl
