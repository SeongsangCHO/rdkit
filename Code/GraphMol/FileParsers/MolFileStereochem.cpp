//
//  Copyright (C) 2004-2021 Greg Landrum and other RDKit contributors
//
//   @@ All Rights Reserved @@
//  This file is part of the RDKit.
//  The contents are covered by the terms of the BSD license
//  which is included in the file license.txt, found at the root
//  of the RDKit source tree.
//
//
#include <list>
#include <RDGeneral/RDLog.h>
#include "MolFileStereochem.h"
#include <Geometry/point.h>
#include <boost/dynamic_bitset.hpp>
#include <algorithm>
#include <RDGeneral/Ranking.h>

namespace RDKit {
typedef std::list<double> DOUBLE_LIST;

void WedgeBond(Bond *bond, unsigned int fromAtomIdx, const Conformer *conf) {
  PRECONDITION(bond, "no bond");
  PRECONDITION(conf, "no conformer");
  PRECONDITION(&conf->getOwningMol() == &bond->getOwningMol(),
               "bond and conformer do not belong to same molecule");
  if (bond->getBondType() != Bond::SINGLE) {
    return;
  }
  Bond::BondDir dir = DetermineBondWedgeState(bond, fromAtomIdx, conf);
  if (dir == Bond::BEGINWEDGE || dir == Bond::BEGINDASH) {
    bond->setBondDir(dir);
  }
}

void WedgeMolBonds(ROMol &mol, const Conformer *conf) {
  PRECONDITION(conf, "no conformer");
  auto wedgeBonds = pickBondsToWedge(mol);
  for (auto bond : mol.bonds()) {
    if (bond->getBondType() == Bond::SINGLE) {
      Bond::BondDir dir = DetermineBondWedgeState(bond, wedgeBonds, conf);
      if (dir == Bond::BEGINWEDGE || dir == Bond::BEGINDASH) {
        bond->setBondDir(dir);

        // it is possible that this
        // wedging was determined by a chiral atom at the end of the
        // bond (instead of at the beginning). In this case we need to
        // reverse the begin and end atoms for the bond
        auto wbi = wedgeBonds.find(bond->getIdx());
        if (wbi != wedgeBonds.end() &&
            static_cast<unsigned int>(wbi->second) != bond->getBeginAtomIdx()) {
          auto tmp = bond->getBeginAtomIdx();
          bond->setBeginAtomIdx(bond->getEndAtomIdx());
          bond->setEndAtomIdx(tmp);
        }
      }
    }
  }
}

std::tuple<unsigned int, unsigned int, unsigned int> getDoubleBondPresence(
    const ROMol &mol, const Atom &atom) {
  unsigned int hasDouble = 0;
  unsigned int hasKnownDouble = 0;
  unsigned int hasAnyDouble = 0;
  for (const auto &nbri : boost::make_iterator_range(mol.getAtomBonds(&atom))) {
    const auto bond = mol[nbri];
    if (bond->getBondType() == Bond::BondType::DOUBLE) {
      ++hasDouble;
      if (bond->getStereo() == Bond::BondStereo::STEREOANY) {
        ++hasAnyDouble;
      } else if (bond->getStereo() > Bond::BondStereo::STEREOANY) {
        ++hasKnownDouble;
      }
    }
  }
  return std::make_tuple(hasDouble, hasKnownDouble, hasAnyDouble);
}

INT_MAP_INT pickBondsToWedge(const ROMol &mol) {
  // we need ring information; make sure findSSSR has been called before
  // if not call now
  if (!mol.getRingInfo()->isInitialized()) {
    MolOps::findSSSR(mol);
  }

  static int noNbrs = 100;
  INT_VECT nChiralNbrs(mol.getNumAtoms(), noNbrs);

  // start by looking for bonds that are already wedged
  for (const auto bond : mol.bonds()) {
    if (bond->getBondDir() == Bond::BEGINWEDGE ||
        bond->getBondDir() == Bond::BEGINDASH ||
        bond->getBondDir() == Bond::UNKNOWN) {
      if (bond->getBeginAtom()->getChiralTag() == Atom::CHI_TETRAHEDRAL_CW ||
          bond->getBeginAtom()->getChiralTag() == Atom::CHI_TETRAHEDRAL_CCW) {
        nChiralNbrs[bond->getBeginAtomIdx()] = noNbrs + 1;
      } else if (bond->getEndAtom()->getChiralTag() ==
                     Atom::CHI_TETRAHEDRAL_CW ||
                 bond->getEndAtom()->getChiralTag() ==
                     Atom::CHI_TETRAHEDRAL_CCW) {
        nChiralNbrs[bond->getEndAtomIdx()] = noNbrs + 1;
      }
    }
  }

  // now rank atoms by the number of chiral neighbors or Hs they have:
  bool chiNbrs = false;
  for (const auto at : mol.atoms()) {
    if (nChiralNbrs[at->getIdx()] > noNbrs) {
      // std::cerr << " SKIPPING1: " << at->getIdx() << std::endl;
      continue;
    }
    Atom::ChiralType type = at->getChiralTag();
    if (type != Atom::CHI_TETRAHEDRAL_CW && type != Atom::CHI_TETRAHEDRAL_CCW) {
      continue;
    }
    nChiralNbrs[at->getIdx()] = 0;
    chiNbrs = true;
    for (const auto nat : mol.atomNeighbors(at)) {
      if (nat->getAtomicNum() == 1) {
        // special case: it's an H... we weight these especially high:
        nChiralNbrs[at->getIdx()] -= 10;
        continue;
      }
      type = nat->getChiralTag();
      if (type != Atom::CHI_TETRAHEDRAL_CW &&
          type != Atom::CHI_TETRAHEDRAL_CCW) {
        continue;
      }
      nChiralNbrs[at->getIdx()] -= 1;
    }
  }
  std::vector<unsigned int> indices(mol.getNumAtoms());
  for (unsigned int i = 0; i < mol.getNumAtoms(); ++i) {
    indices[i] = i;
  }

  if (chiNbrs) {
    std::sort(indices.begin(), indices.end(), [&](auto i1, auto i2) {
      return nChiralNbrs[i1] < nChiralNbrs[i2];
    });
  }
#if 0
  std::cerr << "  nbrs: ";
  std::copy(nChiralNbrs.begin(), nChiralNbrs.end(),
            std::ostream_iterator<int>(std::cerr, " "));
  std::cerr << std::endl;
  std::cerr << "  order: ";
  std::copy(indices.begin(), indices.end(),
            std::ostream_iterator<int>(std::cerr, " "));
  std::cerr << std::endl;
#endif
  // picks a bond for each atom that we will wedge when we write the mol file
  // here is what we are going to do
  // - at each chiral center look for a bond that is begins at the atom and
  //   is not yet picked to be wedged for a different chiral center, preferring
  //   bonds to Hs
  // - if we do not find a bond that begins at the chiral center - we will take
  //   the first bond that is not yet picked by any other chiral centers
  // we use the orders calculated above to determine which order to do the
  // wedging
  INT_MAP_INT res;
  for (auto idx : indices) {
    if (nChiralNbrs[idx] > noNbrs) {
      // std::cerr << " SKIPPING2: " << idx << std::endl;
      continue;  // already have a wedged bond here
    }
    const Atom *atom = mol.getAtomWithIdx(idx);
    Atom::ChiralType type = atom->getChiralTag();
    // the indices are ordered such that all chiral atoms come first. If
    // this has no chiral flag, we can stop the whole loop:
    if (type != Atom::CHI_TETRAHEDRAL_CW && type != Atom::CHI_TETRAHEDRAL_CCW) {
      break;
    }
    std::vector<std::pair<int, int>> nbrScores;
    for (const auto bond : mol.atomBonds(atom)) {
      // can only wedge single bonds:
      if (bond->getBondType() != Bond::SINGLE) {
        continue;
      }

      int bid = bond->getIdx();
      if (res.find(bid) == res.end()) {
        // very strong preference for Hs:
        if (bond->getOtherAtom(atom)->getAtomicNum() == 1) {
          nbrScores.emplace_back(-1000000,
                                 bid);  // lower than anything else can be
          continue;
        }
        // prefer lower atomic numbers with lower degrees and no specified
        // chirality:
        const Atom *oatom = bond->getOtherAtom(atom);
        int nbrScore =
            oatom->getAtomicNum() + 100 * oatom->getDegree() +
            1000 * ((oatom->getChiralTag() != Atom::CHI_UNSPECIFIED));
        // prefer neighbors that are nonchiral or have as few chiral neighbors
        // as possible:
        int oIdx = oatom->getIdx();
        if (nChiralNbrs[oIdx] < noNbrs) {
          // the counts are negative, so we have to subtract them off
          nbrScore -= 100000 * nChiralNbrs[oIdx];
        }

        // prefer bonds to non-ring atoms:
        nbrScore += 10000 * mol.getRingInfo()->numAtomRings(oIdx);
        // prefer non-ring bonds;
        nbrScore += 20000 * mol.getRingInfo()->numBondRings(bid);
        // prefer bonds to atoms which don't have a double bond from them
        unsigned int hasDoubleBond;       // is a double bond there?
        unsigned int hasKnownDoubleBond;  // is specified stereo there?
        unsigned int hasAnyDoubleBond;    // is STEREOANY there?
        std::tie(hasDoubleBond, hasKnownDoubleBond, hasAnyDoubleBond) =
            getDoubleBondPresence(mol, *oatom);
        // NOTE that the relative weightings of ring atoms/bonds and the
        // presence of ring double bonds is only appropriate if we aren't adding
        // a squiggly wedge. We really don't want to add those between
        // chiral centers and double bonds.

        nbrScore += 11000 * hasDoubleBond;
        nbrScore += 12000 * hasKnownDoubleBond;
        nbrScore +=
            23000 * hasAnyDoubleBond;  // we weight these really high in case we
                                       // need to add a squiggly bond to
                                       // indicate the double bond

        // std::cerr << "    nrbScore: " << idx << " - " << oIdx << " : "
        //           << nbrScore << " nChiralNbrs: " << nChiralNbrs[oIdx]
        //           << std::endl;
        nbrScores.emplace_back(nbrScore, bid);
      }
    }
    // There's still one situation where this whole thing can fail: an unlucky
    // situation where all neighbors of all neighbors of an atom are chiral and
    // that atom ends up being the last one picked for stereochem assignment.
    //
    // We'll catch that as an error here and hope that it's as unlikely to occur
    // as it seems like it is. (I'm going into this knowing that it's bound to
    // happen; I'll kick myself and do the hard solution at that point.)
    CHECK_INVARIANT(nbrScores.size(),
                    "no eligible neighbors for chiral center");
    std::sort(nbrScores.begin(), nbrScores.end(), Rankers::pairLess);
    res[nbrScores[0].second] = idx;
  }
  return res;
}

std::vector<Bond *> getBondNeighbors(ROMol &mol, const Bond &bond) {
  std::vector<Bond *> res;
  for (auto nbri :
       boost::make_iterator_range(mol.getAtomBonds(bond.getBeginAtom()))) {
    auto nbrBond = mol[nbri];
    if (nbrBond == &bond) {
      continue;
    }
    res.push_back(nbrBond);
  }
  for (auto nbri :
       boost::make_iterator_range(mol.getAtomBonds(bond.getEndAtom()))) {
    auto nbrBond = mol[nbri];
    if (nbrBond == &bond) {
      continue;
    }
    res.push_back(nbrBond);
  }
  return res;
}

const Atom *getNonsharedAtom(const Bond &bond1, const Bond &bond2) {
  if (bond1.getBeginAtomIdx() == bond2.getBeginAtomIdx() ||
      bond1.getBeginAtomIdx() == bond2.getEndAtomIdx()) {
    return bond1.getEndAtom();
  } else if (bond1.getEndAtomIdx() == bond2.getBeginAtomIdx() ||
             bond1.getEndAtomIdx() == bond2.getEndAtomIdx()) {
    return bond1.getBeginAtom();
  }
  POSTCONDITION(0, "bonds don't share an atom");
}

const unsigned StereoBondThresholds::DBL_BOND_NO_STEREO;
const unsigned StereoBondThresholds::DBL_BOND_SPECIFIED_STEREO;
const unsigned StereoBondThresholds::CHIRAL_ATOM;
const unsigned StereoBondThresholds::DIRECTION_SET;

// a note on the way the StereoBondThresholds are used:
//  the penalties are all 1/10th of the corresponding threshold, so
//  the penalty for being connected to a chiral atom is
//  StereoBondThresholds::CHIRAL_ATOM/10
//  This allows us to just add up the penalties for a particular
//  single bond and still use one set of thresholds - an individual
//  single bond will never have any particular penalty term applied
//  more than a couple of times
//
void addWavyBondsForStereoAny(ROMol &mol, bool clearDoubleBondFlags,
                              unsigned addWhenImpossible) {
  std::vector<int> singleBondScores(mol.getNumBonds(), 0);
  // used to store the double bond neighbors, if any, of each single bond
  std::map<unsigned, std::vector<unsigned>> singleBondNeighbors;
  boost::dynamic_bitset<> doubleBondsToSet(mol.getNumBonds());
  // mark single bonds adjacent to double bonds
  for (const auto dblBond : mol.bonds()) {
    if (dblBond->getBondType() != Bond::BondType::DOUBLE) {
      continue;
    }
    if (dblBond->getStereo() == Bond::BondStereo::STEREOANY) {
      doubleBondsToSet.set(dblBond->getIdx());
    }
    for (auto singleBond : getBondNeighbors(mol, *dblBond)) {
      if (singleBond->getBondType() != Bond::BondType::SINGLE) {
        continue;
      }
      // NOTE: we could make this canonical by initializing scores to the
      // canonical atom ranks
      int score = singleBondScores[singleBond->getIdx()];
      ++score;

      // penalty for having a direction already set
      if (singleBond->getBondDir() != Bond::BondDir::NONE) {
        score += StereoBondThresholds::DIRECTION_SET / 10;
      }

      // penalties from the double bond itself:

      // penalize being adjacent to a double bond with empty stereo:
      if (dblBond->getStereo() == Bond::BondStereo::STEREONONE) {
        score += StereoBondThresholds::DBL_BOND_NO_STEREO / 10;
      } else if (dblBond->getStereo() > Bond::BondStereo::STEREOANY) {
        // penalize being adjacent to a double bond with specified stereo:
        score += StereoBondThresholds::DBL_BOND_SPECIFIED_STEREO / 10;
      }

      // atom-related penalties
      auto otherAtom = getNonsharedAtom(*singleBond, *dblBond);
      // favor atoms with smaller numbers of neighbors:
      score += 10 * otherAtom->getDegree();
      // penalty for being adjacent to an atom with specified stereo
      if (otherAtom->getChiralTag() != Atom::ChiralType::CHI_UNSPECIFIED &&
          otherAtom->getChiralTag() != Atom::ChiralType::CHI_OTHER) {
        score += StereoBondThresholds::CHIRAL_ATOM / 10;
      }
      singleBondScores[singleBond->getIdx()] = score;
      if (dblBond->getStereo() == Bond::BondStereo::STEREOANY) {
        singleBondNeighbors[singleBond->getIdx()].push_back(dblBond->getIdx());
      }
    }
  }
  std::vector<std::tuple<int, unsigned int, size_t>> sortedScores;
  for (size_t i = 0; i < mol.getNumBonds(); ++i) {
    auto score = singleBondScores[i];
    if (!score) {
      continue;
    }
    sortedScores.push_back(
        std::make_tuple(-1 * singleBondNeighbors[i].size(), score, i));
  }
  std::sort(sortedScores.begin(), sortedScores.end());
  for (const auto &tpl : sortedScores) {
    // FIX: check if dir is already set
    for (auto dblBondIdx : singleBondNeighbors[std::get<2>(tpl)]) {
      if (doubleBondsToSet[dblBondIdx]) {
        if (addWhenImpossible) {
          if (std::get<1>(tpl) > addWhenImpossible) {
            continue;
          }
        } else if (std::get<1>(tpl) >
                   StereoBondThresholds::DBL_BOND_NO_STEREO) {
          BOOST_LOG(rdWarningLog)
              << "Setting wavy bond flag on bond " << std::get<2>(tpl)
              << " which may make other stereo info ambiguous" << std::endl;
        }
        mol.getBondWithIdx(std::get<2>(tpl))
            ->setBondDir(Bond::BondDir::UNKNOWN);
        if (clearDoubleBondFlags) {
          auto dblBond = mol.getBondWithIdx(dblBondIdx);
          if (dblBond->getBondDir() == Bond::BondDir::EITHERDOUBLE) {
            dblBond->setBondDir(Bond::BondDir::NONE);
          }
          dblBond->setStereo(Bond::BondStereo::STEREONONE);
        }
        doubleBondsToSet.reset(dblBondIdx);
      }
    }
  }
  if (addWhenImpossible) {
    if (doubleBondsToSet.count()) {
      std::stringstream sstr;
      sstr << " unable to set wavy bonds for double bonds:";
      for (size_t i = 0; i < mol.getNumBonds(); ++i) {
        if (doubleBondsToSet[i]) {
          sstr << " " << i;
        }
      }
      BOOST_LOG(rdWarningLog) << sstr.str() << std::endl;
    }
  }
}
//
// Determine bond wedge state
///
Bond::BondDir DetermineBondWedgeState(const Bond *bond,
                                      unsigned int fromAtomIdx,
                                      const Conformer *conf) {
  PRECONDITION(bond, "no bond");
  PRECONDITION(bond->getBondType() == Bond::SINGLE,
               "bad bond order for wedging");
  const ROMol *mol = &(bond->getOwningMol());
  PRECONDITION(mol, "no mol");

  Bond::BondDir res = bond->getBondDir();
  if (!conf) {
    return res;
  }

  Atom *atom, *bondAtom;  // = bond->getBeginAtom();
  if (bond->getBeginAtom()->getIdx() == fromAtomIdx) {
    atom = bond->getBeginAtom();
    bondAtom = bond->getEndAtom();
  } else {
    atom = bond->getEndAtom();
    bondAtom = bond->getBeginAtom();
  }

  Atom::ChiralType chiralType = atom->getChiralTag();
  CHECK_INVARIANT(chiralType == Atom::CHI_TETRAHEDRAL_CW ||
                      chiralType == Atom::CHI_TETRAHEDRAL_CCW,
                  "");

  // if we got this far, we really need to think about it:
  INT_LIST neighborBondIndices;
  DOUBLE_LIST neighborBondAngles;
  RDGeom::Point3D centerLoc, tmpPt;
  centerLoc = conf->getAtomPos(atom->getIdx());
  tmpPt = conf->getAtomPos(bondAtom->getIdx());
  centerLoc.z = 0.0;
  tmpPt.z = 0.0;
  RDGeom::Point3D refVect = centerLoc.directionVector(tmpPt);

  neighborBondIndices.push_back(bond->getIdx());
  neighborBondAngles.push_back(0.0);

  ROMol::OEDGE_ITER beg, end;
  boost::tie(beg, end) = mol->getAtomBonds(atom);
  while (beg != end) {
    const Bond *nbrBond = (*mol)[*beg];
    Atom *otherAtom = nbrBond->getOtherAtom(atom);
    if (nbrBond != bond) {
      tmpPt = conf->getAtomPos(otherAtom->getIdx());
      tmpPt.z = 0.0;
      RDGeom::Point3D tmpVect = centerLoc.directionVector(tmpPt);
      double angle = refVect.signedAngleTo(tmpVect);
      if (angle < 0.0) {
        angle += 2. * M_PI;
      }
      auto nbrIt = neighborBondIndices.begin();
      auto angleIt = neighborBondAngles.begin();
      // find the location of this neighbor in our angle-sorted list
      // of neighbors:
      while (angleIt != neighborBondAngles.end() && angle > (*angleIt)) {
        ++angleIt;
        ++nbrIt;
      }
      neighborBondAngles.insert(angleIt, angle);
      neighborBondIndices.insert(nbrIt, nbrBond->getIdx());
    }
    ++beg;
  }

  // at this point, neighborBondIndices contains a list of bond
  // indices from the central atom.  They are arranged starting
  // at the reference bond in CCW order (based on the current
  // depiction).
  int nSwaps = atom->getPerturbationOrder(neighborBondIndices);

  // in the case of three-coordinated atoms we may have to worry about
  // the location of the implicit hydrogen - Issue 209
  // Check if we have one of these situation
  //
  //      0        1 0 2
  //      *         \*/
  //  1 - C - 2      C
  //
  // here the hydrogen will be between 1 and 2 and we need to add an additional
  // swap
  if (neighborBondAngles.size() == 3) {
    // three coordinated
    auto angleIt = neighborBondAngles.begin();
    ++angleIt;  // the first is the 0 (or reference bond - we will ignoire that
    double angle1 = (*angleIt);
    ++angleIt;
    double angle2 = (*angleIt);
    if (angle2 - angle1 >= (M_PI - 1e-4)) {
      // we have the above situation
      nSwaps++;
    }
  }

#ifdef VERBOSE_STEREOCHEM
  BOOST_LOG(rdDebugLog) << "--------- " << nSwaps << std::endl;
  std::copy(neighborBondIndices.begin(), neighborBondIndices.end(),
            std::ostream_iterator<int>(BOOST_LOG(rdDebugLog), " "));
  BOOST_LOG(rdDebugLog) << std::endl;
  std::copy(neighborBondAngles.begin(), neighborBondAngles.end(),
            std::ostream_iterator<double>(BOOST_LOG(rdDebugLog), " "));
  BOOST_LOG(rdDebugLog) << std::endl;
#endif
  if (chiralType == Atom::CHI_TETRAHEDRAL_CCW) {
    if (nSwaps % 2 == 1) {  // ^ reverse) {
      res = Bond::BEGINDASH;
    } else {
      res = Bond::BEGINWEDGE;
    }
  } else {
    if (nSwaps % 2 == 1) {  // ^ reverse) {
      res = Bond::BEGINWEDGE;
    } else {
      res = Bond::BEGINDASH;
    }
  }

  return res;
}
Bond::BondDir DetermineBondWedgeState(const Bond *bond,
                                      const INT_MAP_INT &wedgeBonds,
                                      const Conformer *conf) {
  PRECONDITION(bond, "no bond");
  int bid = bond->getIdx();
  auto wbi = wedgeBonds.find(bid);
  if (wbi == wedgeBonds.end()) {
    return bond->getBondDir();
  }

  unsigned int waid = wbi->second;
  return DetermineBondWedgeState(bond, waid, conf);
}

// handles stereochem markers set by the Mol file parser and
// converts them to the RD standard:
void DetectAtomStereoChemistry(RWMol &mol, const Conformer *conf) {
  PRECONDITION(conf, "no conformer");
  PRECONDITION(&(conf->getOwningMol()) == &mol,
               "conformer does not belong to molecule");
  MolOps::assignChiralTypesFromBondDirs(mol, conf->getId(), true);
}

void ClearSingleBondDirFlags(ROMol &mol) {
  for (RWMol::BondIterator bondIt = mol.beginBonds(); bondIt != mol.endBonds();
       ++bondIt) {
    if ((*bondIt)->getBondType() == Bond::SINGLE) {
      if ((*bondIt)->getBondDir() == Bond::UNKNOWN) {
        (*bondIt)->setProp(common_properties::_UnknownStereo, 1);
      }
      (*bondIt)->setBondDir(Bond::NONE);
    }
  }
}

void DetectBondStereoChemistry(ROMol &mol, const Conformer *conf) {
  PRECONDITION(conf, "no conformer");
  PRECONDITION(&(conf->getOwningMol()) == &mol,
               "conformer does not belong to molecule");
  MolOps::detectBondStereochemistry(mol, conf->getId());
}
}  // namespace RDKit
