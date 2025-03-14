//
//  Copyright (C) 2020-2021 Greg Landrum and other RDKit contributors
//
//   @@ All Rights Reserved @@
//  This file is part of the RDKit.
//  The contents are covered by the terms of the BSD license
//  which is included in the file license.txt, found at the root
//  of the RDKit source tree.
//

#include "catch.hpp"

#include <GraphMol/RDKitBase.h>
#include <GraphMol/StereoGroup.h>
#include <GraphMol/Chirality.h>
#include <GraphMol/MolOps.h>

#include <GraphMol/FileParsers/FileParsers.h>
#include <GraphMol/FileParsers/MolFileStereochem.h>
#include <GraphMol/FileParsers/MolSupplier.h>
#include <GraphMol/SmilesParse/SmilesParse.h>
#include <GraphMol/SmilesParse/SmilesWrite.h>

using namespace RDKit;

TEST_CASE("bond StereoInfo", "[unittest]") {
  SECTION("basics") {
    {
      auto mol = "CC=C(C#C)C=C"_smiles;
      REQUIRE(mol);
      auto sinfo = Chirality::detail::getStereoInfo(mol->getBondWithIdx(1));
      CHECK(sinfo.type == Chirality::StereoType::Bond_Double);
      CHECK(sinfo.centeredOn == 1);
      REQUIRE(sinfo.controllingAtoms.size() == 4);
      CHECK(sinfo.controllingAtoms[0] == 0);
      CHECK(sinfo.controllingAtoms[1] == Chirality::StereoInfo::NOATOM);
      CHECK(sinfo.controllingAtoms[2] == 3);
      CHECK(sinfo.controllingAtoms[3] == 5);
      CHECK(sinfo.specified == Chirality::StereoSpecified::Unspecified);
      CHECK(sinfo.descriptor == Chirality::StereoDescriptor::None);
    }
    {
      auto mol = "CC=NC=N"_smiles;
      REQUIRE(mol);
      auto sinfo = Chirality::detail::getStereoInfo(mol->getBondWithIdx(1));
      CHECK(sinfo.type == Chirality::StereoType::Bond_Double);
      CHECK(sinfo.centeredOn == 1);
      REQUIRE(sinfo.controllingAtoms.size() == 4);
      CHECK(sinfo.controllingAtoms[0] == 0);
      CHECK(sinfo.controllingAtoms[1] == Chirality::StereoInfo::NOATOM);
      CHECK(sinfo.controllingAtoms[2] == 3);
      CHECK(sinfo.controllingAtoms[3] == Chirality::StereoInfo::NOATOM);
    }
  }
  SECTION("stereo") {
    {
      auto mol = "C/C=C(/C#C)C"_smiles;
      REQUIRE(mol);

      CHECK(mol->getBondWithIdx(1)->getStereoAtoms().size() == 2);
      CHECK(mol->getBondWithIdx(1)->getStereoAtoms()[0] == 0);
      CHECK(mol->getBondWithIdx(1)->getStereoAtoms()[1] == 3);

      auto sinfo = Chirality::detail::getStereoInfo(mol->getBondWithIdx(1));
      CHECK(sinfo.type == Chirality::StereoType::Bond_Double);
      CHECK(sinfo.centeredOn == 1);
      REQUIRE(sinfo.controllingAtoms.size() == 4);
      CHECK(sinfo.controllingAtoms[0] == 0);
      CHECK(sinfo.controllingAtoms[1] == Chirality::StereoInfo::NOATOM);
      CHECK(sinfo.controllingAtoms[2] == 3);
      CHECK(sinfo.controllingAtoms[3] == 5);
      CHECK(sinfo.specified == Chirality::StereoSpecified::Specified);
      CHECK(sinfo.descriptor == Chirality::StereoDescriptor::Bond_Trans);
    }
    {  // check an example where one of the stereo atoms isn't the first
       // neighbor
      auto mol = "C/C=C(/C)C#C"_smiles;
      REQUIRE(mol);

      CHECK(mol->getBondWithIdx(1)->getStereoAtoms().size() == 2);
      CHECK(mol->getBondWithIdx(1)->getStereoAtoms()[0] == 0);
      CHECK(mol->getBondWithIdx(1)->getStereoAtoms()[1] == 4);

      auto sinfo = Chirality::detail::getStereoInfo(mol->getBondWithIdx(1));
      CHECK(sinfo.type == Chirality::StereoType::Bond_Double);
      CHECK(sinfo.centeredOn == 1);
      REQUIRE(sinfo.controllingAtoms.size() == 4);
      CHECK(sinfo.controllingAtoms[0] == 0);
      CHECK(sinfo.controllingAtoms[1] == Chirality::StereoInfo::NOATOM);
      CHECK(sinfo.controllingAtoms[2] == 3);
      CHECK(sinfo.controllingAtoms[3] == 4);
      CHECK(sinfo.specified == Chirality::StereoSpecified::Specified);
      CHECK(sinfo.descriptor == Chirality::StereoDescriptor::Bond_Trans);
    }
    {
      auto mol = "C/C=C(\\C#C)C"_smiles;
      REQUIRE(mol);

      CHECK(mol->getBondWithIdx(1)->getStereoAtoms().size() == 2);
      CHECK(mol->getBondWithIdx(1)->getStereoAtoms()[0] == 0);
      CHECK(mol->getBondWithIdx(1)->getStereoAtoms()[1] == 3);

      auto sinfo = Chirality::detail::getStereoInfo(mol->getBondWithIdx(1));
      CHECK(sinfo.type == Chirality::StereoType::Bond_Double);
      CHECK(sinfo.centeredOn == 1);
      REQUIRE(sinfo.controllingAtoms.size() == 4);
      CHECK(sinfo.controllingAtoms[0] == 0);
      CHECK(sinfo.controllingAtoms[1] == Chirality::StereoInfo::NOATOM);
      CHECK(sinfo.controllingAtoms[2] == 3);
      CHECK(sinfo.controllingAtoms[3] == 5);
      CHECK(sinfo.specified == Chirality::StereoSpecified::Specified);
      CHECK(sinfo.descriptor == Chirality::StereoDescriptor::Bond_Cis);
    }
    {  // any bonds
      auto mol = "CC=C(C#C)C"_smiles;
      REQUIRE(mol);

      mol->getBondWithIdx(1)->setStereo(Bond::BondStereo::STEREOANY);

      auto sinfo = Chirality::detail::getStereoInfo(mol->getBondWithIdx(1));
      CHECK(sinfo.type == Chirality::StereoType::Bond_Double);
      CHECK(sinfo.centeredOn == 1);
      REQUIRE(sinfo.controllingAtoms.size() == 4);
      CHECK(sinfo.controllingAtoms[0] == 0);
      CHECK(sinfo.controllingAtoms[1] == Chirality::StereoInfo::NOATOM);
      CHECK(sinfo.controllingAtoms[2] == 3);
      CHECK(sinfo.controllingAtoms[3] == 5);
      CHECK(sinfo.specified == Chirality::StereoSpecified::Unknown);
      CHECK(sinfo.descriptor == Chirality::StereoDescriptor::None);
    }
  }
}
TEST_CASE("isBondPotentialStereoBond", "[unittest]") {
  SECTION("basics") {
    {
      auto mol = "CC=C(C#C)C=C"_smiles;
      REQUIRE(mol);
      CHECK(
          Chirality::detail::isBondPotentialStereoBond(mol->getBondWithIdx(1)));
      CHECK(!Chirality::detail::isBondPotentialStereoBond(
          mol->getBondWithIdx(5)));
      CHECK(!Chirality::detail::isBondPotentialStereoBond(
          mol->getBondWithIdx(3)));
      CHECK(!Chirality::detail::isBondPotentialStereoBond(
          mol->getBondWithIdx(4)));
    }
    {
      auto mol = "CC=NC=N"_smiles;
      REQUIRE(mol);
      CHECK(
          Chirality::detail::isBondPotentialStereoBond(mol->getBondWithIdx(1)));
      CHECK(!Chirality::detail::isBondPotentialStereoBond(
          mol->getBondWithIdx(3)));
    }
    {
      SmilesParserParams ps;
      ps.removeHs = false;
      std::unique_ptr<ROMol> mol{SmilesToMol("[H]C=CC=C([H])[H]", ps)};
      REQUIRE(mol);
      CHECK(!Chirality::detail::isBondPotentialStereoBond(
          mol->getBondWithIdx(1)));
      CHECK(!Chirality::detail::isBondPotentialStereoBond(
          mol->getBondWithIdx(3)));
    }
  }
  SECTION("ring size") {
    {
      auto m = "C1=CCCCC1"_smiles;
      REQUIRE(m);
      CHECK(
          !Chirality::detail::isBondPotentialStereoBond(m->getBondWithIdx(0)));
    }
    {
      auto m = "C1=CCCCCC1"_smiles;
      REQUIRE(m);
      CHECK(
          !Chirality::detail::isBondPotentialStereoBond(m->getBondWithIdx(0)));
    }
    {
      auto m = "C12=C(CCCC2)CCCCCC1"_smiles;
      REQUIRE(m);
      CHECK(
          !Chirality::detail::isBondPotentialStereoBond(m->getBondWithIdx(0)));
    }
  }
}

TEST_CASE("atom StereoInfo", "[unittest]") {
  SECTION("basics") {
    {
      auto mol = "CC(F)(Cl)CNC(C)C"_smiles;
      REQUIRE(mol);
      auto sinfo = Chirality::detail::getStereoInfo(mol->getAtomWithIdx(1));
      CHECK(sinfo.type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(sinfo.centeredOn == 1);
      REQUIRE(sinfo.controllingAtoms.size() == 4);
      CHECK(sinfo.controllingAtoms[0] == 0);
      CHECK(sinfo.controllingAtoms[1] == 2);
      CHECK(sinfo.controllingAtoms[2] == 3);
      CHECK(sinfo.controllingAtoms[3] == 4);
      CHECK(sinfo.specified == Chirality::StereoSpecified::Unspecified);
      CHECK(sinfo.descriptor == Chirality::StereoDescriptor::None);

      sinfo = Chirality::detail::getStereoInfo(mol->getAtomWithIdx(6));
      CHECK(sinfo.type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(sinfo.centeredOn == 6);
      REQUIRE(sinfo.controllingAtoms.size() == 3);
      CHECK(sinfo.controllingAtoms[0] == 5);
      CHECK(sinfo.controllingAtoms[1] == 7);
      CHECK(sinfo.controllingAtoms[2] == 8);
      CHECK(sinfo.specified == Chirality::StereoSpecified::Unspecified);
      CHECK(sinfo.descriptor == Chirality::StereoDescriptor::None);
    }

    {
      auto mol = "C[C@](F)(Cl)CNC(C)C"_smiles;
      REQUIRE(mol);
      auto sinfo = Chirality::detail::getStereoInfo(mol->getAtomWithIdx(1));
      CHECK(sinfo.type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(sinfo.centeredOn == 1);
      REQUIRE(sinfo.controllingAtoms.size() == 4);
      CHECK(sinfo.controllingAtoms[0] == 0);
      CHECK(sinfo.controllingAtoms[1] == 2);
      CHECK(sinfo.controllingAtoms[2] == 3);
      CHECK(sinfo.controllingAtoms[3] == 4);
      CHECK(sinfo.specified == Chirality::StereoSpecified::Specified);
      CHECK(sinfo.descriptor == Chirality::StereoDescriptor::Tet_CCW);
    }

    {
      auto mol = "CN1CC1N(F)C"_smiles;
      REQUIRE(mol);
      auto sinfo = Chirality::detail::getStereoInfo(mol->getAtomWithIdx(1));
      CHECK(sinfo.type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(sinfo.centeredOn == 1);
      REQUIRE(sinfo.controllingAtoms.size() == 3);
      CHECK(sinfo.controllingAtoms[0] == 0);
      CHECK(sinfo.controllingAtoms[1] == 2);
      CHECK(sinfo.controllingAtoms[2] == 3);
    }

    {
      auto mol = "O[As](F)C[As]C[As]"_smiles;
      REQUIRE(mol);

      auto sinfo = Chirality::detail::getStereoInfo(mol->getAtomWithIdx(1));
      CHECK(sinfo.type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(sinfo.centeredOn == 1);
      REQUIRE(sinfo.controllingAtoms.size() == 3);
      CHECK(sinfo.controllingAtoms[0] == 0);
      CHECK(sinfo.controllingAtoms[1] == 2);
      CHECK(sinfo.controllingAtoms[2] == 3);

      sinfo = Chirality::detail::getStereoInfo(mol->getAtomWithIdx(4));
      CHECK(sinfo.type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(sinfo.centeredOn == 4);
      REQUIRE(sinfo.controllingAtoms.size() == 2);
      CHECK(sinfo.controllingAtoms[0] == 3);
      CHECK(sinfo.controllingAtoms[1] == 5);
    }
  }
}
TEST_CASE("isAtomPotentialTetrahedralCenter", "[unittest]") {
  SECTION("basics") {
    {
      auto mol = "CC(F)(Cl)CNC(C)(C)C"_smiles;
      REQUIRE(mol);
      CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(1)));
      CHECK(!Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(0)));
      CHECK(!Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(4)));
      CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(6)));
    }
    {
      auto mol = "CN1CC1N(F)C"_smiles;
      REQUIRE(mol);
      CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(1)));
      CHECK(!Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(4)));
    }
    {
      auto mol = "O=S(F)CC[S+]([O-])CS=O"_smiles;
      REQUIRE(mol);
      CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(1)));
      CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(5)));
      CHECK(!Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(8)));
    }
    {
      auto mol = "O=[Se](F)CC[Se+]([O-])C[Se]=O"_smiles;
      REQUIRE(mol);
      CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(1)));
      CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(5)));
      CHECK(!Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(8)));
    }
    {
      auto mol = "OP(F)CPCP"_smiles;
      REQUIRE(mol);
      CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(1)));
      CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(4)));
      CHECK(!Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(6)));
    }
    {
      auto mol = "O[As](F)C[As]C[As]"_smiles;
      REQUIRE(mol);
      CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(1)));
      CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(4)));
      CHECK(!Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(6)));
    }
    {
      auto mol = "O[P]([O-])(=O)OC"_smiles;
      REQUIRE(mol);
      CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(
          mol->getAtomWithIdx(1)));
    }
  }
}
TEST_CASE("isAtomPotentialStereoAtom", "[unittest]") {
  SECTION("basics") {
    {
      auto mol = "CC(F)(Cl)CNC(C)(C)C"_smiles;
      REQUIRE(mol);
      for (const auto atom : mol->atoms()) {
        CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(atom) ==
              Chirality::detail::isAtomPotentialStereoAtom(atom));
      }
    }
    {
      auto mol = "CN1CC1N(F)C"_smiles;
      REQUIRE(mol);
      for (const auto atom : mol->atoms()) {
        CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(atom) ==
              Chirality::detail::isAtomPotentialStereoAtom(atom));
      }
    }
    {
      auto mol = "O=S(F)CC[S+]([O-])CS=O"_smiles;
      REQUIRE(mol);
      for (const auto atom : mol->atoms()) {
        CHECK(Chirality::detail::isAtomPotentialTetrahedralCenter(atom) ==
              Chirality::detail::isAtomPotentialStereoAtom(atom));
      }
    }
  }
}

TEST_CASE("possible stereochemistry on atoms", "[chirality]") {
  SECTION("specified") {
    {
      auto mol = "CC(C)(O)[C@](Cl)(F)I"_smiles;
      REQUIRE(mol);
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 1);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Specified);
      CHECK(stereoInfo[0].centeredOn == 4);
      std::vector<unsigned> catoms = {1, 5, 6, 7};
      CHECK(stereoInfo[0].controllingAtoms == catoms);
    }
    {
      auto mol = "C[C@@H](O)[C@H](C)[C@H](C)O"_smiles;
      REQUIRE(mol);
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 3);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Specified);
      CHECK(stereoInfo[0].centeredOn == 1);

      CHECK(stereoInfo[1].type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Specified);
      CHECK(stereoInfo[1].centeredOn == 3);

      CHECK(stereoInfo[2].type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(stereoInfo[2].specified == Chirality::StereoSpecified::Specified);
      CHECK(stereoInfo[2].centeredOn == 5);
    }

    {
      auto mol = "FC(F)(F)[C@@H](O)[C@H](C)[C@H](C(F)(F)F)O"_smiles;
      REQUIRE(mol);
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 3);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Specified);
      CHECK(stereoInfo[0].centeredOn == 4);

      CHECK(stereoInfo[1].type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Specified);
      CHECK(stereoInfo[1].centeredOn == 6);

      CHECK(stereoInfo[2].type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(stereoInfo[2].specified == Chirality::StereoSpecified::Specified);
      CHECK(stereoInfo[2].centeredOn == 8);
    }
  }
  SECTION("simple unspecified") {
    {
      auto mol = "CC(C)(O)C(Cl)(F)I"_smiles;
      REQUIRE(mol);

      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 1);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unspecified);
      CHECK(stereoInfo[0].centeredOn == 4);
      std::vector<unsigned> catoms = {1, 5, 6, 7};
      CHECK(stereoInfo[0].controllingAtoms == catoms);
    }
  }
  SECTION("atoms with unknown set, real") {
    auto mol = "FC(O)C"_smiles;
    REQUIRE(mol);
    mol->getBondBetweenAtoms(0, 1)->setBondDir(Bond::BondDir::UNKNOWN);
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    REQUIRE(stereoInfo.size() == 1);
    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unknown);
    CHECK(stereoInfo[0].centeredOn == 1);
  }
  SECTION("atoms with unknown set, not real") {
    auto mol = "CC(O)C"_smiles;
    REQUIRE(mol);
    mol->getBondBetweenAtoms(0, 1)->setBondDir(Bond::BondDir::UNKNOWN);
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    CHECK(stereoInfo.size() == 0);
  }
  SECTION("Isotopes") {
    {
      auto mol = "O[C@H](F)[18OH]"_smiles;
      REQUIRE(mol);
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 1);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Specified);
      CHECK(stereoInfo[0].centeredOn == 1);
      std::vector<unsigned> catoms = {0, 2, 3};
      CHECK(stereoInfo[0].controllingAtoms == catoms);
    }
  }
}

TEST_CASE("possible stereochemistry on bonds", "[chirality]") {
  SECTION("simplest") {
    {
      auto mol = "CC=CC"_smiles;
      REQUIRE(mol);
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 1);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Bond_Double);
      CHECK(stereoInfo[0].centeredOn == 1);
      std::vector<unsigned> catoms = {0, Chirality::StereoInfo::NOATOM, 3,
                                      Chirality::StereoInfo::NOATOM};
      CHECK(stereoInfo[0].controllingAtoms == catoms);
    }
    {
      auto mol = "CC=C(C)C"_smiles;
      REQUIRE(mol);
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      CHECK(stereoInfo.size() == 0);
    }
    {
      auto mol = "CC=C"_smiles;
      REQUIRE(mol);
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      CHECK(stereoInfo.size() == 0);
    }
    {
      auto mol = "CC(F)=C(Cl)C"_smiles;
      REQUIRE(mol);
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 1);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Bond_Double);
      CHECK(stereoInfo[0].centeredOn == 2);
      std::vector<unsigned> catoms = {0, 2, 4, 5};
      CHECK(stereoInfo[0].controllingAtoms == catoms);
    }
    {
      auto mol = "CC=C(Cl)C"_smiles;
      REQUIRE(mol);
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 1);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Bond_Double);
      CHECK(stereoInfo[0].centeredOn == 1);
      std::vector<unsigned> catoms = {0, Chirality::StereoInfo::NOATOM, 3, 4};
      CHECK(stereoInfo[0].controllingAtoms == catoms);
    }
  }
  SECTION("bond with unknown set, real") {
    auto mol = "CC=C(C)F"_smiles;
    REQUIRE(mol);
    mol->getBondWithIdx(1)->setStereo(Bond::BondStereo::STEREOANY);
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    REQUIRE(stereoInfo.size() == 1);
    CHECK(stereoInfo[0].type == Chirality::StereoType::Bond_Double);
    CHECK(stereoInfo[0].centeredOn == 1);
    CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unknown);
  }
  SECTION("bond with unknown set, not real") {
    auto mol = "CC=C(C)C"_smiles;
    REQUIRE(mol);
    mol->getBondWithIdx(1)->setStereo(Bond::BondStereo::STEREOANY);
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    CHECK(stereoInfo.size() == 0);
  }
}

TEST_CASE("para-stereocenters and assignStereochemistry", "[chirality]") {
  SECTION("simplest") {
    auto mol = "CC(F)C(C)C(C)F"_smiles;
    REQUIRE(mol);
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    REQUIRE(stereoInfo.size() == 3);

    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].centeredOn == 1);
    CHECK(stereoInfo[0].controllingAtoms.size() == 3);
    CHECK(stereoInfo[1].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[1].centeredOn == 3);
    CHECK(stereoInfo[1].controllingAtoms.size() == 3);
    CHECK(stereoInfo[2].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[2].centeredOn == 5);
    CHECK(stereoInfo[2].controllingAtoms.size() == 3);
  }

  SECTION("including bonds") {
    // thanks to Salome Rieder for this nasty example
    auto mol = "CC=CC(C=CC)C(C)C(C=CC)C=CC"_smiles;
    REQUIRE(mol);
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    CHECK(stereoInfo.size() == 7);

    std::sort(stereoInfo.begin(), stereoInfo.end(),
              [](const Chirality::StereoInfo &a,
                 const Chirality::StereoInfo &b) -> bool {
                return (a.type < b.type) && (a.centeredOn < b.centeredOn) &&
                       (a.specified < b.specified) &&
                       (a.descriptor < b.descriptor) &&
                       (a.controllingAtoms < b.controllingAtoms);
              });
    REQUIRE(stereoInfo.size() == 7);

    CHECK(stereoInfo[6].type == Chirality::StereoType::Bond_Double);
    CHECK(stereoInfo[6].centeredOn == 13);
    CHECK(stereoInfo[5].type == Chirality::StereoType::Bond_Double);
    CHECK(stereoInfo[5].centeredOn == 10);
    CHECK(stereoInfo[4].type == Chirality::StereoType::Bond_Double);
    CHECK(stereoInfo[4].centeredOn == 4);
    CHECK(stereoInfo[3].type == Chirality::StereoType::Bond_Double);
    CHECK(stereoInfo[3].centeredOn == 1);

    CHECK(stereoInfo[2].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[2].centeredOn == 9);
    CHECK(stereoInfo[1].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[1].centeredOn == 7);
    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].centeredOn == 3);
  }

  SECTION("sugar fun") {
    auto mol = "C1(O)C(O)C(O)C(O)C(O)C1O"_smiles;
    REQUIRE(mol);
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    REQUIRE(stereoInfo.size() == 6);
    for (const auto &si : stereoInfo) {
      CHECK(si.type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(si.centeredOn % 2 == 0);
      CHECK(si.specified == Chirality::StereoSpecified::Unspecified);
    }
  }
}

TEST_CASE("ring stereochemistry", "[chirality]") {
  SECTION("specified") {
    auto mol = "C[C@H]1CC[C@@H](C)CC1"_smiles;
    REQUIRE(mol);
    // std::cerr << "------------ 1 -------------" << std::endl;

    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    REQUIRE(stereoInfo.size() == 2);
    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].centeredOn == 1);
    CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Specified);
    CHECK(stereoInfo[1].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[1].centeredOn == 4);
    CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Specified);
  }
  SECTION("unspecified") {
    auto mol = "CC1CCC(C)CC1"_smiles;
    REQUIRE(mol);
    // std::cerr << "------------ 2 -------------" << std::endl;
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    REQUIRE(stereoInfo.size() == 2);
    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].centeredOn == 1);
    CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unspecified);

    CHECK(stereoInfo[1].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[1].centeredOn == 4);
    CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Unspecified);
  }
  SECTION("four ring") {
    auto mol = "C[C@H]1C[C@@H](C)C1"_smiles;
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    REQUIRE(stereoInfo.size() == 2);
    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].centeredOn == 1);
    CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Specified);

    CHECK(stereoInfo[1].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[1].centeredOn == 3);
    CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Specified);
  }
  SECTION("four ring unspecified") {
    auto mol = "CC1CC(C)C1"_smiles;
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    REQUIRE(stereoInfo.size() == 2);
    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].centeredOn == 1);
    CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unspecified);

    CHECK(stereoInfo[1].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[1].centeredOn == 3);
    CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Unspecified);
  }
}
#if 0
// FIX: the double bond stereo in rings isn't working. This also fails with the canonicalizer, so it's not unique to this code
TEST_CASE("tricky recursive example from Dan Nealschneider", "[chirality]") {
  SECTION("adapted") {
    auto mol = "CC=C1CCC(O)CC1"_smiles;
    REQUIRE(mol);
    mol->updatePropertyCache();
    MolOps::setBondStereoFromDirections(*mol);
    std::cerr << "------------ 1 -------------" << std::endl;
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    REQUIRE(stereoInfo.size() == 2);
    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].centeredOn == 5);
    CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Specified);
    CHECK(stereoInfo[1].type == Chirality::StereoType::Bond_Double);
    CHECK(stereoInfo[1].centeredOn == 1);
    CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Specified);
  }
  SECTION("simplified") {
    // can't sanitize this because the current (2020.03) assignStereochemistry
    // code doesn't recognize the stereo here and removes it
    SmilesParserParams ps;
    ps.sanitize = false;
    ps.removeHs = false;
    std::unique_ptr<ROMol> mol(SmilesToMol("C/C=C1/C[C@H](O)C1", ps));
    REQUIRE(mol);
    mol->updatePropertyCache();
    MolOps::setBondStereoFromDirections(*mol);
    std::cerr << "------------ 2 -------------" << std::endl;
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    REQUIRE(stereoInfo.size() == 2);
    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].centeredOn == 4);
    CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Specified);
    CHECK(stereoInfo[1].type == Chirality::StereoType::Bond_Double);
    CHECK(stereoInfo[1].centeredOn == 1);
    CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Specified);
  }
  // FIX this still isn't working
  SECTION("unspecified") {
    auto mol = "CC=C1C[CH](O)C1"_smiles;
    REQUIRE(mol);
    std::cerr << "------------ 3 -------------" << std::endl;
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    REQUIRE(stereoInfo.size() == 2);
    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].centeredOn == 4);
    CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unspecified);
    CHECK(stereoInfo[1].type == Chirality::StereoType::Bond_Double);
    CHECK(stereoInfo[1].centeredOn == 1);
    CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Unspecified);
  }
}
#endif

TEST_CASE("unknown stereo", "[chirality]") {
  SECTION("atoms") {
    auto mol = "CC(O)C[C@@H](O)F"_smiles;
    REQUIRE(mol);
    REQUIRE(mol->getBondBetweenAtoms(0, 1));
    mol->getBondBetweenAtoms(0, 1)->setBondDir(Bond::BondDir::UNKNOWN);
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    REQUIRE(stereoInfo.size() == 2);
    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].centeredOn == 1);
    CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unknown);
    CHECK(stereoInfo[1].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[1].centeredOn == 4);
    CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Specified);
  }
  SECTION("atoms2") {
    // artificial situation: "squiggly bond" overrides the specified atomic
    // stereo
    auto mol = "C[C@H](O)C[C@@H](O)F"_smiles;
    REQUIRE(mol);
    REQUIRE(mol->getBondBetweenAtoms(0, 1));
    mol->getBondBetweenAtoms(0, 1)->setBondDir(Bond::BondDir::UNKNOWN);
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    REQUIRE(stereoInfo.size() == 2);
    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].centeredOn == 1);
    CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unknown);
    CHECK(stereoInfo[1].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[1].centeredOn == 4);
    CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Specified);
  }
  SECTION("bonds") {
    {
      auto mol = "CC=CC"_smiles;
      REQUIRE(mol);
      REQUIRE(mol->getBondBetweenAtoms(1, 2));
      mol->getBondBetweenAtoms(1, 2)->setBondDir(Bond::BondDir::EITHERDOUBLE);
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 1);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Bond_Double);
      CHECK(stereoInfo[0].centeredOn == 1);
      CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unknown);
    }
    {
      auto mol = "CC=CC=C"_smiles;
      REQUIRE(mol);
      REQUIRE(mol->getBondBetweenAtoms(1, 2));
      mol->getBondBetweenAtoms(1, 2)->setBondDir(Bond::BondDir::EITHERDOUBLE);
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 1);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Bond_Double);
      CHECK(stereoInfo[0].centeredOn == 1);
      CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unknown);
    }
  }
  SECTION("bonds with squiggle bonds") {
    {  // to begin atom
      auto mol = "CC=CC"_smiles;
      REQUIRE(mol);
      REQUIRE(mol->getBondBetweenAtoms(0, 1));
      mol->getBondBetweenAtoms(0, 1)->setBondDir(Bond::BondDir::UNKNOWN);
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 1);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Bond_Double);
      CHECK(stereoInfo[0].centeredOn == 1);
      CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unknown);
    }
    {  // to end atom
      auto mol = "CC=CC"_smiles;
      REQUIRE(mol);
      REQUIRE(mol->getBondBetweenAtoms(2, 3));
      mol->getBondBetweenAtoms(2, 3)->setBondDir(Bond::BondDir::UNKNOWN);
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 1);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Bond_Double);
      CHECK(stereoInfo[0].centeredOn == 1);
      CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unknown);
    }
  }
}

TEST_CASE("cleaning chirality", "[chirality]") {
  SECTION("atoms") {
    auto mol = "CC(O)C"_smiles;
    REQUIRE(mol);
    mol->getAtomWithIdx(1)->setChiralTag(Atom::ChiralType::CHI_TETRAHEDRAL_CW);
    {
      // by default we don't clean up, so the chiral center survives even though
      // we don't get any results:
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      CHECK(stereoInfo.size() == 0);
      CHECK(mol->getAtomWithIdx(1)->getChiralTag() ==
            Atom::ChiralType::CHI_TETRAHEDRAL_CW);
    }
    {
      bool cleanIt = true;
      auto stereoInfo = Chirality::findPotentialStereo(*mol, cleanIt);
      CHECK(stereoInfo.size() == 0);
      CHECK(mol->getAtomWithIdx(1)->getChiralTag() ==
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
  SECTION("bonds") {
    auto mol = "CC=C(C)C"_smiles;
    REQUIRE(mol);
    mol->getBondWithIdx(1)->setStereoAtoms(0, 3);
    mol->getBondWithIdx(1)->setStereo(Bond::BondStereo::STEREOCIS);
    {
      // by default we don't clean up, so the stereo bond survives even though
      // we don't get any results:
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      CHECK(stereoInfo.size() == 0);
      CHECK(mol->getBondWithIdx(1)->getStereo() == Bond::BondStereo::STEREOCIS);
    }
    {
      bool cleanIt = true;
      auto stereoInfo = Chirality::findPotentialStereo(*mol, cleanIt);
      CHECK(stereoInfo.size() == 0);
      CHECK(mol->getBondWithIdx(1)->getStereo() ==
            Bond::BondStereo::STEREONONE);
    }
  }
}

TEST_CASE("flagPossible", "[chirality]") {
  SECTION("atoms") {
    auto mol = "CC(O)[C@H](F)O"_smiles;
    REQUIRE(mol);
    {
      // by default we do use flagPossible:
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 2);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(stereoInfo[0].centeredOn == 1);
      CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unspecified);
      CHECK(stereoInfo[1].type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(stereoInfo[1].centeredOn == 3);
      CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Specified);
    }
    {
      bool cleanIt = false;
      bool flagPossible = false;
      auto stereoInfo =
          Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
      REQUIRE(stereoInfo.size() == 1);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(stereoInfo[0].centeredOn == 3);
      CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Specified);
    }
  }
  SECTION("bonds") {
    auto mol = "CC=C/C=C/C"_smiles;
    REQUIRE(mol);
    {
      // by default we do use flagPossible
      auto stereoInfo = Chirality::findPotentialStereo(*mol);
      REQUIRE(stereoInfo.size() == 2);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Bond_Double);
      CHECK(stereoInfo[0].centeredOn == 1);
      CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unspecified);
      CHECK(stereoInfo[1].type == Chirality::StereoType::Bond_Double);
      CHECK(stereoInfo[1].centeredOn == 3);
      CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Specified);
    }
    {
      bool cleanIt = true;
      bool flagPossible = false;
      auto stereoInfo =
          Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
      REQUIRE(stereoInfo.size() == 1);
      CHECK(stereoInfo[0].type == Chirality::StereoType::Bond_Double);
      CHECK(stereoInfo[0].centeredOn == 3);
      CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Specified);
    }
  }
}

TEST_CASE("cleanup after removing possible centers", "[chirality]") {
  SECTION("atoms1") {
    auto mol = "FC(Cl)(F)C(C(Cl)(F)F)I"_smiles;
    REQUIRE(mol);
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    CHECK(stereoInfo.empty());
  }
  SECTION("bonds1") {
    auto mol = "FC(Cl)(F)C(C(Cl)(F)F)=CF"_smiles;
    REQUIRE(mol);
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    CHECK(stereoInfo.empty());
  }
  SECTION("atoms2") {
    auto mol = "ClC(F)(F)C(=CC(F)C=C(C(F)(F)Cl)C(F)(F)Cl)C(Cl)(F)F"_smiles;
    REQUIRE(mol);
    auto stereoInfo = Chirality::findPotentialStereo(*mol);
    CHECK(stereoInfo.empty());
  }
}

TEST_CASE("findPotentialStereo problems related to #3490", "[chirality][bug]") {
  SECTION("example 1") {
    auto mol = "CC1CC(O)C1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].centeredOn == 1);
    CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unspecified);
    CHECK(stereoInfo[1].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[1].centeredOn == 3);
    CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Unspecified);
  }
  SECTION("example 2a") {
    auto mol = "C(C(C)C1)C12CCN2"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
  }
  SECTION("example 2b") {
    auto mol = "CC(C1)CC12CCN2"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
  }
  SECTION("example 2c") {
    auto mol = "C([C@H](C)C1)[C@]12CCN2"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
  }
  SECTION("example 2d") {
    auto mol = "C[C@H](C1)C[C@]12CCN2"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
  }
  SECTION("example 3") {
    auto mol = "C(C(C)C1)C12CN(C3)CCCCC23"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 4);  // [1, 4, 6, 12]
    CHECK(stereoInfo[0].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[0].centeredOn == 1);
    CHECK(stereoInfo[0].specified == Chirality::StereoSpecified::Unspecified);
    CHECK(stereoInfo[1].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[1].centeredOn == 4);
    CHECK(stereoInfo[1].specified == Chirality::StereoSpecified::Unspecified);
    CHECK(stereoInfo[2].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[2].centeredOn == 6);
    CHECK(stereoInfo[2].specified == Chirality::StereoSpecified::Unspecified);
    CHECK(stereoInfo[3].type == Chirality::StereoType::Atom_Tetrahedral);
    CHECK(stereoInfo[3].centeredOn == 12);
    CHECK(stereoInfo[3].specified == Chirality::StereoSpecified::Unspecified);
  }
}
TEST_CASE("ring stereo finding is overly aggressive", "[chirality][bug]") {
  SECTION("Finding too much 1a") {
    auto mol = "CC1CCCCC1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 0);
  }
  SECTION("Finding too much 1b") {
    auto mol = "CC1CCC(C)CC1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
  }
  SECTION("Finding too much 1c") {
    auto mol = "C[C@H]1CCC(C)CC1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
  }
  SECTION("Finding too much 1d") {
    auto mol = "CC1(C)CCCCC1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 0);
  }
  SECTION("Finding too much 1e") {
    auto mol = "CC1(C)CCC(C)CC1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 0);
  }
  SECTION("Finding too much 1f") {
    auto mol = "C2CC2C1(C2CC2)CCCCC1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 0);
  }
  SECTION("Finding too much 1g") {
    auto mol = "CC1CC2(CCC2)C1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 0);
  }
  SECTION("Finding too much 1h") {
    auto mol = "CC1CC2(CC(C)C2)C1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 3);
  }

  SECTION("Finding too much 2a") {
    auto mol = "CC1CCNCC1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 0);
  }
  SECTION("Finding too much 2b") {
    auto mol = "CC1CCN(C)CC1"_smiles;  // 3-coordinate N is not stereogenic
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 0);
  }
  SECTION("Finding too much 3a") {
    auto mol = "CC1CCC1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 0);
  }

  SECTION("Finding too much 3b") {
    auto mol = "CC1CC(C)C1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
  }
  SECTION("fused rings 1") {
    auto mol = "C1CCC2CCCCC2C1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
  }

  SECTION("fused rings 2") {
    auto mol = "C1CC2CCCC2C1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
  }

  SECTION("cages 1") {
    auto mol = "CC1CN2CCC1CC2"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 3);
    CHECK(stereoInfo[0].centeredOn == 1);
    CHECK(stereoInfo[1].centeredOn == 3);
    CHECK(stereoInfo[2].centeredOn == 6);
  }
  SECTION("cages 1b") {
    auto mol = "O1CN2CCC1CC2"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
    CHECK(stereoInfo[0].centeredOn == 2);
    CHECK(stereoInfo[1].centeredOn == 5);
  }
  SECTION("cages 2") {
    auto mol = "C1CC2(O)CCC1(C)CC2"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
    CHECK(stereoInfo[0].centeredOn == 2);
    CHECK(stereoInfo[1].centeredOn == 6);
  }
  SECTION("cages 3") {
    auto mol = "C1CC2(O)CCC1CC2"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
    CHECK(stereoInfo[0].centeredOn == 2);
    CHECK(stereoInfo[1].centeredOn == 6);
  }
  SECTION("adamantyl") {
    auto mol = "CC12CC3CC(CC(C3)C1)C2"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 4);
  }
  SECTION("bug 1a") {
    // example that came up during testing
    auto mol = "C(=O)C(C(C)N2C=C2)C(=O)"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    REQUIRE(stereoInfo.size() == 1);
    CHECK(stereoInfo[0].centeredOn == 3);
  }
  SECTION("bug 1b") {
    // example that came up during testing
    auto mol = "C(=O)C(C(CC)c2ccc(Cl)cc2)C(=O)"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    REQUIRE(stereoInfo.size() == 1);
    CHECK(stereoInfo[0].centeredOn == 3);
  }

  SECTION("bug 1c") {
    // example that came up during testing
    auto mol = "O=CC(C=O)C(C)n2cccc2"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    REQUIRE(stereoInfo.size() == 1);
    CHECK(stereoInfo[0].centeredOn == 5);
  }

  SECTION("bug 1c") {
    // example that came up during testing
    auto mol = "C(=O)C(C(C)n2cccc2)C(=O)"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    REQUIRE(stereoInfo.size() == 1);
    CHECK(stereoInfo[0].centeredOn == 3);
  }

  SECTION("bug 1d") {
    // example that came up during testing
    auto mol = "C(O)C(C(C)n2cccc2)C(O)"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    REQUIRE(stereoInfo.size() == 1);
    CHECK(stereoInfo[0].centeredOn == 3);
  }
  SECTION("just a bug") {
    // example that came up during testing

    auto mol = "CC1=CN(C2OC(CNC(=O)C3c4ccccc4Sc4ccccc43)CC2)C(=O)NC1=O"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    auto stereoInfo =
        Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
    CHECK(stereoInfo.size() == 2);
  }
}

TEST_CASE(
    "github #3631: Ring stereochemistry not properly removed from N atoms",
    "[chirality][bug]") {
  SECTION("basics") {
    SmilesParserParams ps;
    ps.sanitize = false;
    ps.removeHs = false;
    std::unique_ptr<RWMol> mol{SmilesToMol("C[N@]1C[C@@](F)(Cl)C1", ps)};
    REQUIRE(mol);
    MolOps::sanitizeMol(*mol);

    CHECK(mol->getAtomWithIdx(1)->getChiralTag() !=
          Atom::ChiralType::CHI_UNSPECIFIED);
    CHECK(mol->getAtomWithIdx(3)->getChiralTag() !=
          Atom::ChiralType::CHI_UNSPECIFIED);
    bool cleanIt = true;
    bool flagPossible = true;
    bool force = true;
    {
      RWMol mol2(*mol);
      auto stereoInfo =
          Chirality::findPotentialStereo(mol2, cleanIt, flagPossible);
      CHECK(stereoInfo.size() == 0);
    }
    {
      RWMol mol2(*mol);
      MolOps::assignStereochemistry(mol2, cleanIt, force, flagPossible);
      CHECK(mol2.getAtomWithIdx(1)->getChiralTag() ==
            Atom::ChiralType::CHI_UNSPECIFIED);
      CHECK(mol2.getAtomWithIdx(3)->getChiralTag() ==
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
  SECTION("default behavior") {
    auto mol = "C[N@]1C[C@@](F)(Cl)C1"_smiles;
    REQUIRE(mol);
    auto smiles = MolToSmiles(*mol);
    CHECK(smiles == "CN1CC(F)(Cl)C1");
    bool cleanIt = true;
    bool flagPossible = true;
    bool force = true;
    CHECK(mol->getAtomWithIdx(1)->getChiralTag() ==
          Atom::ChiralType::CHI_UNSPECIFIED);
    CHECK(mol->getAtomWithIdx(3)->getChiralTag() ==
          Atom::ChiralType::CHI_UNSPECIFIED);
    {
      RWMol mol2(*mol);
      auto stereoInfo =
          Chirality::findPotentialStereo(mol2, cleanIt, flagPossible);
      CHECK(stereoInfo.size() == 0);
    }
    {
      RWMol mol2(*mol);
      MolOps::assignStereochemistry(mol2, cleanIt, force, flagPossible);
      CHECK(mol2.getAtomWithIdx(1)->getChiralTag() ==
            Atom::ChiralType::CHI_UNSPECIFIED);
      CHECK(mol2.getAtomWithIdx(3)->getChiralTag() ==
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
  SECTION("don't overcorrect") {
    auto mol = "C[N@]1O[C@@](F)(Cl)C1"_smiles;
    REQUIRE(mol);
    bool cleanIt = true;
    bool flagPossible = true;
    bool force = true;
    {
      RWMol mol2(*mol);
      auto stereoInfo =
          Chirality::findPotentialStereo(mol2, cleanIt, flagPossible);
      CHECK(stereoInfo.size() == 1);
      CHECK(stereoInfo[0].centeredOn == 3);
    }
    {
      RWMol mol2(*mol);
      MolOps::assignStereochemistry(mol2, cleanIt, force, flagPossible);
      CHECK(mol2.getAtomWithIdx(1)->getChiralTag() ==
            Atom::ChiralType::CHI_UNSPECIFIED);
      CHECK(mol2.getAtomWithIdx(3)->getChiralTag() !=
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
}

TEST_CASE("N Chirality in rings") {
  SECTION("basics 4 coordinate") {
    {
      auto mol = "CC1CC2CC[N@@+]1(C)OC2"_smiles;
      REQUIRE(mol);
      CHECK(mol->getAtomWithIdx(6)->getAtomicNum() == 7);
      CHECK(mol->getAtomWithIdx(6)->getChiralTag() !=
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
    {
      auto mol = "C[N@@+](F)(Cl)O"_smiles;
      REQUIRE(mol);
      CHECK(mol->getAtomWithIdx(1)->getAtomicNum() == 7);
      CHECK(mol->getAtomWithIdx(1)->getChiralTag() !=
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
  SECTION("basics 3 coordinate") {
    {
      auto mol = "CC1CC2CC[N@@]1OC2"_smiles;
      REQUIRE(mol);
      CHECK(mol->getAtomWithIdx(6)->getAtomicNum() == 7);
      CHECK(mol->getAtomWithIdx(6)->getChiralTag() !=
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
    {
      auto mol = "C1CC[N@]2OCCCC2C1"_smiles;
      REQUIRE(mol);
      CHECK(mol->getAtomWithIdx(3)->getAtomicNum() == 7);
      CHECK(mol->getAtomWithIdx(3)->getChiralTag() ==
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
  SECTION("ring stereo") {
    {  // real chirality
      auto mol = "C[C@H]1CC[N@@+](C)(O)OC1"_smiles;
      REQUIRE(mol);
      CHECK(mol->getAtomWithIdx(4)->getAtomicNum() == 7);
      CHECK(mol->getAtomWithIdx(4)->getChiralTag() !=
            Atom::ChiralType::CHI_UNSPECIFIED);
      CHECK(mol->getAtomWithIdx(1)->getAtomicNum() == 6);
      CHECK(mol->getAtomWithIdx(1)->getChiralTag() !=
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
    {  // ring stereo
      auto mol = "C[C@H]1CC[N@@+](C)(O)CC1"_smiles;
      REQUIRE(mol);
      CHECK(mol->getAtomWithIdx(4)->getAtomicNum() == 7);
      CHECK(mol->getAtomWithIdx(4)->getChiralTag() !=
            Atom::ChiralType::CHI_UNSPECIFIED);
      CHECK(mol->getAtomWithIdx(1)->getAtomicNum() == 6);
      CHECK(mol->getAtomWithIdx(1)->getChiralTag() !=
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
    {  // three-ring degree-three ring stereo
      auto mol = "C[C@H]1[C@@H](C)[N@]1C"_smiles;
      REQUIRE(mol);
      CHECK(mol->getAtomWithIdx(4)->getAtomicNum() == 7);
      CHECK(mol->getAtomWithIdx(4)->getChiralTag() !=
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
    {  // CHEMBL79374
      auto mol = "Cn1ncc([C@]23CC[N@](CC2)C3)n1"_smiles;
      REQUIRE(mol);
      CHECK(mol->getAtomWithIdx(8)->getAtomicNum() == 7);
      CHECK(mol->getAtomWithIdx(8)->getChiralTag() !=
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
    {  // derived from CHEMBL79374
      auto mol = "Cn1ncc([C@]23CC[C@](CC2)C3)n1"_smiles;
      REQUIRE(mol);
      CHECK(mol->getAtomWithIdx(8)->getAtomicNum() == 6);
      CHECK(mol->getAtomWithIdx(8)->getChiralTag() !=
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
}

TEST_CASE(
    "Github #4115: RemoveStereochemistry should also remove stereogroups") {
  SECTION("basics") {
    auto mol = "C[C@H](O)[C@@H](C)F |o1:1,3,r|"_smiles;
    REQUIRE(mol);
    CHECK(mol->getAtomWithIdx(1)->getChiralTag() !=
          Atom::ChiralType::CHI_UNSPECIFIED);
    CHECK(mol->getAtomWithIdx(3)->getChiralTag() !=
          Atom::ChiralType::CHI_UNSPECIFIED);
    CHECK(mol->getStereoGroups().size() == 1);
    MolOps::removeStereochemistry(*mol);
    CHECK(mol->getAtomWithIdx(1)->getChiralTag() ==
          Atom::ChiralType::CHI_UNSPECIFIED);
    CHECK(mol->getAtomWithIdx(3)->getChiralTag() ==
          Atom::ChiralType::CHI_UNSPECIFIED);
    CHECK(mol->getStereoGroups().empty());
  }
}

TEST_CASE(
    "Github #4155: Problem finding stereocenters in bridged bicyclics with "
    "4-rings") {
  SECTION("specified") {
    std::vector<std::string> smis = {
        "C[C@H]1CC[C@H](CC1)C(N)=O", "C[C@]12CC[C@](CC1)(C2)C(N)=O",
        "C[C@H]1C[C@H](C1)C(N)=O", "C[C@]12C[C@](C1)(CC2)C(N)=O"};
    for (const auto &smi : smis) {
      std::unique_ptr<ROMol> mol(SmilesToMol(smi));
      REQUIRE(mol);
      bool cleanIt = true;
      bool flagPossible = true;
      auto stereoInfo =
          Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
      REQUIRE(stereoInfo.size() == 2);
      CHECK(stereoInfo[0].centeredOn == 1);
    }
  }
  SECTION("unspecified") {
    std::vector<std::string> smis = {
        "CC1CCC(CC1)C(N)=O", "CC12CCC(CC1)(C2)C(N)=O", "CC1CC(C1)C(N)=O",
        "CC12CC(C1)(CC2)C(N)=O"};
    for (const auto &smi : smis) {
      std::unique_ptr<ROMol> mol(SmilesToMol(smi));
      REQUIRE(mol);
      bool cleanIt = true;
      bool flagPossible = true;
      auto stereoInfo =
          Chirality::findPotentialStereo(*mol, cleanIt, flagPossible);
      REQUIRE(stereoInfo.size() == 2);
      CHECK(stereoInfo[0].centeredOn == 1);
    }
  }
}

TEST_CASE("pickBondsToWedge() should avoid double bonds") {
  SECTION("simplest") {
    auto mol = "OC=C[C@H](C1CC1)C2CCC2"_smiles;
    REQUIRE(mol);
    auto wedgedBonds = pickBondsToWedge(*mol);
    REQUIRE(wedgedBonds.size() == 1);
    auto head = wedgedBonds.begin();
    CHECK(head->first == 3);
    CHECK(head->second == 3);
  }
  SECTION("simplest, specified double bond") {
    auto mol = "OC=C[C@H](C1CC1)C2CCC2"_smiles;
    REQUIRE(mol);
    mol->getBondBetweenAtoms(1, 2)->setStereoAtoms(0, 3);
    mol->getBondBetweenAtoms(1, 2)->setStereo(Bond::BondStereo::STEREOCIS);
    auto wedgedBonds = pickBondsToWedge(*mol);
    REQUIRE(wedgedBonds.size() == 1);
    auto head = wedgedBonds.begin();
    CHECK(head->first == 3);
    CHECK(head->second == 3);
  }
  SECTION("prefer unspecified bond stereo") {
    auto mol = "OC=C[C@H](C=CF)(C=CC)"_smiles;
    REQUIRE(mol);
    mol->getBondBetweenAtoms(1, 2)->setStereoAtoms(0, 3);
    mol->getBondBetweenAtoms(1, 2)->setStereo(Bond::BondStereo::STEREOCIS);
    mol->getBondBetweenAtoms(4, 5)->setStereoAtoms(3, 6);
    mol->getBondBetweenAtoms(4, 5)->setStereo(Bond::BondStereo::STEREOANY);
    auto wedgedBonds = pickBondsToWedge(*mol);
    REQUIRE(wedgedBonds.size() == 1);
    auto head = wedgedBonds.begin();
    CHECK(head->first == 6);
    CHECK(head->second == 3);
  }
}

TEST_CASE("addWavyBondsForStereoAny()") {
  SECTION("simplest") {
    auto mol = "CC=CC"_smiles;
    REQUIRE(mol);
    mol->getBondWithIdx(1)->setStereoAtoms(0, 3);
    mol->getBondWithIdx(1)->setStereo(Bond::BondStereo::STEREOANY);
    addWavyBondsForStereoAny(*mol);
    CHECK(mol->getBondWithIdx(0)->getBondDir() == Bond::BondDir::UNKNOWN);
    CHECK(mol->getBondWithIdx(1)->getStereo() == Bond::BondStereo::STEREONONE);
  }
  SECTION("don't reset flags") {
    auto mol = "CC=CC"_smiles;
    REQUIRE(mol);
    mol->getBondWithIdx(1)->setStereoAtoms(0, 3);
    mol->getBondWithIdx(1)->setStereo(Bond::BondStereo::STEREOANY);
    bool clearFlags = false;
    addWavyBondsForStereoAny(*mol, clearFlags);
    CHECK(mol->getBondWithIdx(0)->getBondDir() == Bond::BondDir::UNKNOWN);
    CHECK(mol->getBondWithIdx(1)->getStereo() == Bond::BondStereo::STEREOANY);
  }
  SECTION("avoid double bonds") {
    auto mol = "CC=CC(CC)=CC"_smiles;
    REQUIRE(mol);
    mol->getBondWithIdx(5)->setStereoAtoms(2, 7);
    mol->getBondWithIdx(5)->setStereo(Bond::BondStereo::STEREOANY);
    addWavyBondsForStereoAny(*mol);
    CHECK(mol->getBondWithIdx(6)->getBondDir() == Bond::BondDir::UNKNOWN);
    CHECK(mol->getBondWithIdx(5)->getStereo() == Bond::BondStereo::STEREONONE);
  }
  SECTION("avoid chiral atoms") {
    auto mol = "C[C@](F)(Cl)C(C)=CC"_smiles;
    REQUIRE(mol);
    mol->getBondWithIdx(5)->setStereoAtoms(1, 7);
    mol->getBondWithIdx(5)->setStereo(Bond::BondStereo::STEREOANY);
    addWavyBondsForStereoAny(*mol);
    CHECK(mol->getBondWithIdx(4)->getBondDir() == Bond::BondDir::UNKNOWN);
    CHECK(mol->getBondWithIdx(5)->getStereo() == Bond::BondStereo::STEREONONE);
  }
  SECTION("prefer atoms with less neighbors") {
    auto mol = "CC(F)(Cl)C(CF)=CC"_smiles;
    REQUIRE(mol);
    mol->getBondWithIdx(6)->setStereoAtoms(1, 8);
    mol->getBondWithIdx(6)->setStereo(Bond::BondStereo::STEREOANY);
    addWavyBondsForStereoAny(*mol);
    CHECK(mol->getBondWithIdx(7)->getBondDir() == Bond::BondDir::UNKNOWN);
    CHECK(mol->getBondWithIdx(6)->getStereo() == Bond::BondStereo::STEREONONE);
  }
  SECTION("more complex") {
    auto mol = "CC=CC(C=CO)=CC"_smiles;
    REQUIRE(mol);
    mol->getBondWithIdx(6)->setStereoAtoms(2, 8);
    mol->getBondWithIdx(6)->setStereo(Bond::BondStereo::STEREOANY);
    addWavyBondsForStereoAny(*mol);
    CHECK(mol->getBondWithIdx(7)->getBondDir() == Bond::BondDir::UNKNOWN);
    CHECK(mol->getBondWithIdx(6)->getStereo() == Bond::BondStereo::STEREONONE);
  }
  SECTION("no solution without changing threshold") {
    auto mol = "CC=CC=CC=CC"_smiles;
    REQUIRE(mol);
    mol->getBondWithIdx(1)->setStereoAtoms(0, 3);
    mol->getBondWithIdx(1)->setStereo(Bond::BondStereo::STEREOCIS);
    mol->getBondWithIdx(3)->setStereoAtoms(2, 5);
    mol->getBondWithIdx(3)->setStereo(Bond::BondStereo::STEREOANY);
    mol->getBondWithIdx(5)->setStereoAtoms(4, 7);
    mol->getBondWithIdx(5)->setStereo(Bond::BondStereo::STEREOCIS);
    addWavyBondsForStereoAny(*mol);
    // we didn't actually do anything:
    CHECK(mol->getBondWithIdx(2)->getBondDir() == Bond::BondDir::NONE);
    CHECK(mol->getBondWithIdx(3)->getStereo() == Bond::BondStereo::STEREOANY);

    bool clearDoubleBondFlags = true;
    addWavyBondsForStereoAny(*mol, clearDoubleBondFlags,
                             StereoBondThresholds::DBL_BOND_SPECIFIED_STEREO);
    CHECK(mol->getBondWithIdx(2)->getBondDir() == Bond::BondDir::UNKNOWN);
    CHECK(mol->getBondWithIdx(3)->getStereo() == Bond::BondStereo::STEREONONE);
  }
  SECTION("multiple bonds to wedge") {
    auto mol = "CCC(C)=CC=C(CC)C=CC(C)=CC"_smiles;
    REQUIRE(mol);
    mol->getBondWithIdx(3)->setStereoAtoms(3, 5);
    mol->getBondWithIdx(3)->setStereo(Bond::BondStereo::STEREOCIS);
    mol->getBondWithIdx(9)->setStereoAtoms(6, 11);
    mol->getBondWithIdx(9)->setStereo(Bond::BondStereo::STEREOANY);
    mol->getBondWithIdx(5)->setStereoAtoms(4, 7);
    mol->getBondWithIdx(5)->setStereo(Bond::BondStereo::STEREOANY);
    addWavyBondsForStereoAny(*mol);
    CHECK(mol->getBondWithIdx(9)->getStereo() == Bond::BondStereo::STEREONONE);
    CHECK(mol->getBondWithIdx(5)->getStereo() == Bond::BondStereo::STEREONONE);
    CHECK(mol->getBondWithIdx(8)->getBondDir() == Bond::BondDir::UNKNOWN);
    for (const auto bond : mol->bonds()) {
      if (bond->getBondType() == Bond::BondType::SINGLE &&
          bond->getIdx() != 8) {
        CHECK(bond->getBondDir() == Bond::BondDir::NONE);
      }
    }
  }
}

TEST_CASE("Github #4215: Ring stereo being discarded in spiro systems") {
  // Note: this bug is still there when using the legacy stereochemistry
  // assignment. It's "non-trivial" to fix there and we've opted not to
  SmilesParserParams ps;
  ps.useLegacyStereo = false;
  SECTION("original failing example") {
    std::unique_ptr<RWMol> m{
        SmilesToMol("C[C@H]1CCC2(CC1)CC[C@H](C)C(C)C2", ps)};
    REQUIRE(m);
    CHECK(m->getAtomWithIdx(1)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    CHECK(m->getAtomWithIdx(9)->getChiralTag() != Atom::CHI_UNSPECIFIED);
  }
  SECTION("original passing example") {
    std::unique_ptr<RWMol> m{SmilesToMol("C[C@H]1CCC2(CC1)CC[C@H](C)CC2", ps)};
    REQUIRE(m);
    // if the middle is unspecified, the two ends can't be specified
    CHECK(m->getAtomWithIdx(1)->getChiralTag() == Atom::CHI_UNSPECIFIED);
    CHECK(m->getAtomWithIdx(9)->getChiralTag() == Atom::CHI_UNSPECIFIED);

    {
      bool cleanIt = true;
      bool flagPossible = true;
      RWMol m2(*m);
      auto stereoInfo =
          Chirality::findPotentialStereo(m2, cleanIt, flagPossible);
      CHECK(stereoInfo.size() == 3);
      for (const auto &si : stereoInfo) {
        CHECK(si.type == Chirality::StereoType::Atom_Tetrahedral);
        CHECK(si.specified == Chirality::StereoSpecified::Unspecified);
        CHECK(si.descriptor == Chirality::StereoDescriptor::None);
      }
    }
    {
      bool cleanIt = true;
      bool flagPossible = false;
      RWMol m2(*m);
      auto stereoInfo =
          Chirality::findPotentialStereo(m2, cleanIt, flagPossible);
      CHECK(stereoInfo.empty());
    }
  }
  SECTION("specified chirality on spiro atom") {
    std::unique_ptr<RWMol> m{
        SmilesToMol("C[C@H]1CC[C@@]2(CC[C@H](C)CC2)CC1", ps)};
    REQUIRE(m);
    // now the middle is specified, so the two ends are as well
    CHECK(m->getAtomWithIdx(1)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    CHECK(m->getAtomWithIdx(7)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    CHECK(m->getAtomWithIdx(4)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    {
      bool cleanIt = true;
      bool flagPossible = true;
      RWMol m2(*m);
      auto stereoInfo =
          Chirality::findPotentialStereo(m2, cleanIt, flagPossible);
      CHECK(stereoInfo.size() == 3);
      for (const auto &si : stereoInfo) {
        CHECK(si.type == Chirality::StereoType::Atom_Tetrahedral);
        CHECK(si.specified == Chirality::StereoSpecified::Specified);
        CHECK(si.descriptor != Chirality::StereoDescriptor::None);
      }
    }
    {
      bool cleanIt = true;
      bool flagPossible = false;
      RWMol m2(*m);
      auto stereoInfo =
          Chirality::findPotentialStereo(m2, cleanIt, flagPossible);
      CHECK(stereoInfo.size() == 3);
      for (const auto &si : stereoInfo) {
        CHECK(si.type == Chirality::StereoType::Atom_Tetrahedral);
        CHECK(si.specified == Chirality::StereoSpecified::Specified);
        CHECK(si.descriptor != Chirality::StereoDescriptor::None);
      }
    }
  }
  SECTION("three spiro rings, unspecified spiro links") {
    std::unique_ptr<RWMol> m{
        SmilesToMol("C[C@H]1CCC2(CC1)CCC1(CC[C@H](C)CC1)CC2", ps)};
    REQUIRE(m);
    {
      bool cleanIt = true;
      bool flagPossible = true;
      RWMol m2(*m);
      auto stereoInfo =
          Chirality::findPotentialStereo(m2, cleanIt, flagPossible);
      CHECK(stereoInfo.size() == 4);
      for (const auto &si : stereoInfo) {
        CHECK(si.type == Chirality::StereoType::Atom_Tetrahedral);
        CHECK(si.specified == Chirality::StereoSpecified::Unspecified);
        CHECK(si.descriptor == Chirality::StereoDescriptor::None);
      }
    }
  }
  SECTION("three spiro rings, specified spiro links") {
    std::unique_ptr<RWMol> m{
        SmilesToMol("C[C@H]1CC[C@@]2(CC1)CC[C@]1(CC[C@H](C)CC1)CC2", ps)};
    REQUIRE(m);
    CHECK(m->getAtomWithIdx(1)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    CHECK(m->getAtomWithIdx(4)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    CHECK(m->getAtomWithIdx(9)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    CHECK(m->getAtomWithIdx(12)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    {
      bool cleanIt = true;
      bool flagPossible = true;
      RWMol m2(*m);
      auto stereoInfo =
          Chirality::findPotentialStereo(m2, cleanIt, flagPossible);
      CHECK(stereoInfo.size() == 4);
      for (const auto &si : stereoInfo) {
        CHECK(si.type == Chirality::StereoType::Atom_Tetrahedral);
        CHECK(si.specified == Chirality::StereoSpecified::Specified);
        CHECK(si.descriptor != Chirality::StereoDescriptor::None);
      }
    }
  }
}

TEST_CASE(
    "Github #4279: FindPotentialStereo() doesn't find *marked* ring stereo "
    "when flagPossible=False") {
  SECTION("base") {
    std::unique_ptr<RWMol> m{SmilesToMol("C[C@H]1CC[C@@H](C)CC1")};
    REQUIRE(m);
    CHECK(m->getAtomWithIdx(1)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    CHECK(m->getAtomWithIdx(4)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    bool cleanIt = true;
    bool flagPossible = false;
    auto stereoInfo = Chirality::findPotentialStereo(*m, cleanIt, flagPossible);
    for (const auto &si : stereoInfo) {
      CHECK(si.type == Chirality::StereoType::Atom_Tetrahedral);
      CHECK(si.specified == Chirality::StereoSpecified::Specified);
      CHECK(si.descriptor != Chirality::StereoDescriptor::None);
    }
    CHECK(m->getAtomWithIdx(1)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    CHECK(m->getAtomWithIdx(4)->getChiralTag() != Atom::CHI_UNSPECIFIED);
  }
}

TEST_CASE("StereoInfo comparisons") {
  Chirality::StereoInfo si1;
  si1.centeredOn = 3;
  CHECK(si1.type == Chirality::StereoType::Unspecified);
  si1.type = Chirality::StereoType::Atom_Tetrahedral;
  Chirality::StereoInfo si2;
  si2.centeredOn = 3;
  si2.type = Chirality::StereoType::Atom_Tetrahedral;
  CHECK(si1 == si2);
  si2.descriptor = Chirality::StereoDescriptor::Tet_CCW;
  CHECK(si1 != si2);
}

TEST_CASE("StereoGroup Testing") {
  SECTION("basics") {
    auto mol = "C[C@H](O)[C@@H](C)[C@H](F)Cl |o1:1,3,&2:5,r|"_smiles;
    REQUIRE(mol);
    CHECK(mol->getStereoGroups().size() == 2);
    StereoGroup cp(mol->getStereoGroups()[0]);
    CHECK(cp == mol->getStereoGroups()[0]);
    CHECK(cp != mol->getStereoGroups()[1]);

    std::vector<Atom *> toRemove{mol->getAtomWithIdx(1)};
    std::vector<StereoGroup> &sgs =
        const_cast<std::vector<StereoGroup> &>(mol->getStereoGroups());
    removeGroupsWithAtoms(toRemove, sgs);
    CHECK(mol->getStereoGroups().size() == 1);
  }
}

TEST_CASE("Removing stereogroups from unspecified atoms") {
  SECTION("basics") {
    auto mol = "C[C@](O)(Cl)F |o1:1|"_smiles;
    REQUIRE(mol);
    CHECK(mol->getStereoGroups().size() == 1);
    mol->getAtomWithIdx(1)->setChiralTag(Atom::ChiralType::CHI_UNSPECIFIED);
    Chirality::cleanupStereoGroups(*mol);
    CHECK(mol->getStereoGroups().empty());
  }
  SECTION("parsing") {
    auto mol = "C[C@](C)(Cl)F |o1:1|"_smiles;
    REQUIRE(mol);
    CHECK(mol->getStereoGroups().empty());
  }
  SECTION("partial group removal") {
    auto mol = "C[C@](C)(Cl)[C@H](F)Cl |o1:1,4|"_smiles;
    REQUIRE(mol);
    CHECK(mol->getStereoGroups().size() == 1);
    CHECK(mol->getStereoGroups()[0].getAtoms().size() == 1);
    CHECK(mol->getStereoGroups()[0].getAtoms()[0]->getIdx() == 4);
  }
}

TEST_CASE("replaceAtom and StereoGroups") {
  SECTION("basics") {
    auto mol = "C[C@](O)(Cl)[C@H](F)Cl |o1:1,4|"_smiles;
    REQUIRE(mol);
    CHECK(mol->getStereoGroups().size() == 1);
    CHECK(mol->getStereoGroups()[0].getAtoms().size() == 2);
    CHECK(mol->getStereoGroups()[0].getAtoms()[0] == mol->getAtomWithIdx(1));

    Atom acp(*mol->getAtomWithIdx(1));
    mol->replaceAtom(1, &acp);
    CHECK(mol->getStereoGroups().size() == 1);
    CHECK(mol->getStereoGroups()[0].getAtoms().size() == 2);
    CHECK(mol->getStereoGroups()[0].getAtoms()[0] == mol->getAtomWithIdx(1));
  }
}

TEST_CASE(
    "Github #5200: FindPotentialStereo does not clean stereoflags from atoms "
    "which cannot be stereocenters") {
  auto m = "CCF"_smiles;
  REQUIRE(m);
  m->getAtomWithIdx(1)->setChiralTag(Atom::ChiralType::CHI_TETRAHEDRAL_CCW);
  bool cleanIt = true;
  auto sinfo = Chirality::findPotentialStereo(*m, cleanIt);
  CHECK(sinfo.empty());
  CHECK(m->getAtomWithIdx(1)->getChiralTag() ==
        Atom::ChiralType::CHI_UNSPECIFIED);
}

TEST_CASE(
    "Github #5196: Zero & coordinate bonds are being taken into account for "
    "chirality") {
  RDLog::LogStateSetter setter;  // disable irritating warning messages
  auto mol = R"CTAB(
     RDKit          3D

  0  0  0  0  0  0  0  0  0  0999 V3000
M  V30 BEGIN CTAB
M  V30 COUNTS 15 18 0 0 0
M  V30 BEGIN ATOM
M  V30 1 C -0.136359 0.025241 -0.986870 0
M  V30 2 C 0.211183 -0.810922 0.138318 0
M  V30 3 C -0.446638 -0.713741 1.305561 0
M  V30 4 C -1.141107 0.914647 -0.916429 0
M  V30 5 R -1.628248 -0.983190 -0.411960 0
M  V30 6 H 0.392055 -0.106505 -1.920607 0
M  V30 7 H 0.974038 -1.568492 0.017171 0
M  V30 8 H -0.209921 -1.406535 2.084966 0
M  V30 9 H -1.378909 1.482059 -1.807349 0
M  V30 10 C -1.544607 0.306162 1.588191 0
M  V30 11 C -1.946856 1.186683 0.358271 0
M  V30 12 H -1.207983 0.944410 2.407927 0
M  V30 13 H -2.419549 -0.225146 1.965589 0
M  V30 14 H -3.006492 1.040978 0.144313 0
M  V30 15 H -1.830875 2.240146 0.620809 0
M  V30 END ATOM
M  V30 BEGIN BOND
M  V30 1 1 2 1
M  V30 2 2 3 2
M  V30 3 2 4 1
M  V30 4 0 1 5
M  V30 5 0 2 5
M  V30 6 0 3 5
M  V30 7 0 4 5
M  V30 8 1 1 6
M  V30 9 1 2 7
M  V30 10 1 3 8
M  V30 11 1 4 9
M  V30 12 1 10 3
M  V30 13 1 11 4
M  V30 14 1 11 10
M  V30 15 1 12 10
M  V30 16 1 13 10
M  V30 17 1 14 11
M  V30 18 1 15 11
M  V30 END BOND
M  V30 END CTAB
M  END)CTAB"_ctab;
  REQUIRE(mol);
  SECTION("as reported") {
    MolOps::assignStereochemistryFrom3D(*mol);
    for (auto aidx : {0, 1, 2, 3}) {
      CHECK(mol->getAtomWithIdx(aidx)->getChiralTag() ==
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
  SECTION("as reported - ZOBs") {
    for (auto bidx : {3, 4, 5, 6}) {
      mol->getBondWithIdx(bidx)->setBondType(Bond::BondType::ZERO);
    }
    MolOps::assignStereochemistryFrom3D(*mol);
    for (auto idx : {0, 1, 2, 3}) {
      CHECK(mol->getAtomWithIdx(idx)->getChiralTag() ==
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
  SECTION("as reported - datives") {
    for (auto bidx : {3, 4, 5, 6}) {
      mol->getBondWithIdx(bidx)->setBondType(Bond::BondType::DATIVE);
    }
    MolOps::assignStereochemistryFrom3D(*mol);
    for (auto idx : {0, 1, 2, 3}) {
      CHECK(mol->getAtomWithIdx(idx)->getChiralTag() ==
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
  SECTION("as reported - reversed datives") {
    // structure is bogus, but we want to test
    for (auto bidx : {3, 4, 5, 6}) {
      auto bond = mol->getBondWithIdx(bidx);
      bond->setEndAtomIdx(bond->getBeginAtomIdx());
      bond->setBeginAtomIdx(4);
      bond->setBondType(Bond::BondType::DATIVE);
    }
    MolOps::assignStereochemistryFrom3D(*mol);
    for (auto idx : {0, 1, 2, 3}) {
      CHECK(mol->getAtomWithIdx(idx)->getChiralTag() !=
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
  SECTION("as reported - singles") {
    // structure is bogus, but we want to test
    for (auto bidx : {3, 4, 5, 6}) {
      mol->getBondWithIdx(bidx)->setBondType(Bond::BondType::SINGLE);
    }
    MolOps::assignStereochemistryFrom3D(*mol);
    for (auto idx : {0, 1, 2, 3}) {
      CHECK(mol->getAtomWithIdx(idx)->getChiralTag() !=
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
  SECTION("assignStereochemistry") {
    auto mol = "[Fe]C(=C)O |C:1.0|"_smiles;
    REQUIRE(mol);
    for (auto bt : {Bond::BondType::DATIVE, Bond::BondType::ZERO,
                    Bond::BondType::UNSPECIFIED}) {
      mol->getAtomWithIdx(1)->setChiralTag(
          Atom::ChiralType::CHI_TETRAHEDRAL_CW);
      mol->getBondWithIdx(0)->setBondType(bt);
      bool cleanit = true;
      bool force = true;
      MolOps::assignStereochemistry(*mol, cleanit, force);
      CHECK(mol->getAtomWithIdx(1)->getChiralTag() ==
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
  SECTION("isAtomPotentialTetrahedralCenter() and getStereoInfo()") {
    auto mol = "[Fe]C(=C)O |C:1.0|"_smiles;
    REQUIRE(mol);
    for (auto bt : {Bond::BondType::DATIVE, Bond::BondType::ZERO,
                    Bond::BondType::UNSPECIFIED}) {
      mol->getAtomWithIdx(1)->setChiralTag(
          Atom::ChiralType::CHI_TETRAHEDRAL_CW);
      mol->getBondWithIdx(0)->setBondType(bt);
      CHECK(!Chirality::detail::isAtomPotentialStereoAtom(
          mol->getAtomWithIdx(1)));
      bool cleanit = true;
      auto sinfo = Chirality::findPotentialStereo(*mol, cleanit);
      CHECK(sinfo.empty());
      CHECK(mol->getAtomWithIdx(1)->getChiralTag() ==
            Atom::ChiralType::CHI_UNSPECIFIED);
    }
  }
}

TEST_CASE(
    "Github #5239: Precondition violation on chiral Atoms with zero order "
    "bonds") {
  RDLog::LogStateSetter setter;  // disable irritating warning messages
  auto molblock = R"CTAB(
     RDKit          3D
     
  0  0  0  0  0  0  0  0  0  0999 V3000
M  V30 BEGIN CTAB
M  V30 COUNTS 5 4 0 0 0
M  V30 BEGIN ATOM
M  V30 1 C -0.446600 -0.713700 1.305600 0
M  V30 2 Fe -1.628200 -0.983200 -0.412000 0
M  V30 3 Cl -0.049300 -1.876700 2.613900 0
M  V30 4 C -1.544600 0.306200 1.588200 0
M  V30 5 F 0.673700 0.029200 0.993700 0
M  V30 END ATOM
M  V30 BEGIN BOND
M  V30 1 1 1 3
M  V30 2 1 1 4 CFG=1
M  V30 3 1 1 5
M  V30 4 0 2 1
M  V30 END BOND
M  V30 END CTAB
M  END)CTAB";
  bool sanitize = false;
  std::unique_ptr<ROMol> mol(MolBlockToMol(molblock, sanitize));
  REQUIRE(mol);
  MolOps::assignStereochemistryFrom3D(*mol);

  CHECK(mol->getAtomWithIdx(0)->getChiralTag() !=
        Atom::ChiralType::CHI_UNSPECIFIED);
}
TEST_CASE("nontetrahedral stereo from 3D", "[nontetrahedral]") {
  std::string pathName = getenv("RDBASE");
  pathName += "/Code/GraphMol/test_data/nontetrahedral_3d.sdf";
  SECTION("basics") {
    SDMolSupplier suppl(pathName);
    while (!suppl.atEnd()) {
      std::unique_ptr<ROMol> m{suppl.next()};
      REQUIRE(m);
      MolOps::assignChiralTypesFrom3D(*m);
      auto ct = m->getProp<std::string>("ChiralType");
      auto cp = m->getProp<unsigned>("ChiralPermutation");
      auto atom = m->getAtomWithIdx(0);

      if (ct == "SP") {
        CHECK(atom->getChiralTag() == Atom::ChiralType::CHI_SQUAREPLANAR);
      } else if (ct == "TB") {
        CHECK(atom->getChiralTag() ==
              Atom::ChiralType::CHI_TRIGONALBIPYRAMIDAL);
      } else if (ct == "TH") {
        CHECK(atom->getChiralTag() == Atom::ChiralType::CHI_TETRAHEDRAL);
      } else if (ct == "OH") {
        CHECK(atom->getChiralTag() == Atom::ChiralType::CHI_OCTAHEDRAL);
      }

      CHECK(atom->getProp<unsigned>(common_properties::_chiralPermutation) ==
            cp);
    }
  }
  SECTION("disable nontetrahedral stereo") {
    Chirality::setAllowNontetrahedralChirality(false);
    SDMolSupplier suppl(pathName);
    while (!suppl.atEnd()) {
      std::unique_ptr<ROMol> m{suppl.next()};
      REQUIRE(m);
      MolOps::assignChiralTypesFrom3D(*m);
      auto ct = m->getProp<std::string>("ChiralType");
      auto atom = m->getAtomWithIdx(0);

      if (ct == "TH") {
        CHECK(atom->getChiralTag() == Atom::ChiralType::CHI_TETRAHEDRAL);
      } else {
        CHECK(atom->getChiralTag() == Atom::ChiralType::CHI_UNSPECIFIED);
      }
    }
    Chirality::setAllowNontetrahedralChirality(true);
  }
}

TEST_CASE("assignStereochemistry shouldn't remove nontetrahedral stereo",
          "[nontetrahedral]") {
  SECTION("basics") {
    SmilesParserParams parseps;
    parseps.sanitize = false;
    parseps.removeHs = false;
    std::unique_ptr<RWMol> m{SmilesToMol("F[Pt@TB1](O)(Br)(N)Cl", parseps)};
    REQUIRE(m);
    CHECK(m->getAtomWithIdx(1)->getChiralTag() ==
          Atom::ChiralType::CHI_TRIGONALBIPYRAMIDAL);
    bool cleanIt = true;
    bool force = true;
    MolOps::assignStereochemistry(*m, cleanIt, force);
    CHECK(m->getAtomWithIdx(1)->getChiralTag() ==
          Atom::ChiralType::CHI_TRIGONALBIPYRAMIDAL);
  }
  SECTION("standard SMILES parsing") {
    std::unique_ptr<RWMol> m{SmilesToMol("F[Pt@TB1](O)(Br)(N)Cl")};
    REQUIRE(m);
    CHECK(m->getAtomWithIdx(1)->getChiralTag() ==
          Atom::ChiralType::CHI_TRIGONALBIPYRAMIDAL);
  }
  SECTION("SMILES parsing w/o sanitization") {
    SmilesParserParams parseps;
    // we need to skip stereo assignment
    parseps.sanitize = false;
    std::unique_ptr<RWMol> m{SmilesToMol("F[Pt@TB1](O)(Br)(N)Cl", parseps)};
    REQUIRE(m);
    CHECK(m->getAtomWithIdx(1)->getChiralTag() ==
          Atom::ChiralType::CHI_TRIGONALBIPYRAMIDAL);
  }
}

TEST_CASE("remove hs and non-tetrahedral stereo", "[nontetrahedral]") {
  SmilesParserParams parseps;
  parseps.sanitize = false;
  parseps.removeHs = false;
  std::vector<std::string> smiles = {"F[Pt@TB1]([H])(Br)(N)Cl",
                                     "F[Pt@TB]([H])(Br)(N)Cl"};
  for (const auto &smi : smiles) {
    std::unique_ptr<RWMol> m{SmilesToMol(smi, parseps)};
    REQUIRE(m);
    CHECK(m->getNumAtoms(6));
    {
      // the default is to not remove Hs to non-tetrahedral stereocenters
      RWMol molcp(*m);
      MolOps::removeHs(molcp);
      CHECK(molcp.getNumAtoms() == 6);
    }
    {
      // but we can enable it
      RWMol molcp(*m);
      MolOps::RemoveHsParameters ps;
      ps.removeNontetrahedralNeighbors = true;
      MolOps::removeHs(molcp, ps);
      CHECK(molcp.getNumAtoms() == 5);
    }
    {
      // but we can enable it
      RWMol molcp(*m);
      MolOps::removeAllHs(molcp);
      CHECK(molcp.getNumAtoms() == 5);
    }
  }
}

TEST_CASE("getIdealAngle", "[nontetrahedral]") {
  SECTION("TB1") {
    auto m = "S[As@TB1](F)(Cl)(Br)N"_smiles;
    REQUIRE(m);
    CHECK(Chirality::isTrigonalBipyramidalAxialAtom(m->getAtomWithIdx(1),
                                                    m->getAtomWithIdx(0)));
    CHECK(Chirality::isTrigonalBipyramidalAxialAtom(m->getAtomWithIdx(1),
                                                    m->getAtomWithIdx(5)));
    CHECK(!Chirality::isTrigonalBipyramidalAxialAtom(m->getAtomWithIdx(1),
                                                     m->getAtomWithIdx(2)));
    CHECK(!Chirality::isTrigonalBipyramidalAxialAtom(m->getAtomWithIdx(1),
                                                     m->getAtomWithIdx(3)));
    CHECK(!Chirality::isTrigonalBipyramidalAxialAtom(m->getAtomWithIdx(1),
                                                     m->getAtomWithIdx(4)));
    CHECK(Chirality::getTrigonalBipyramidalAxialAtom(m->getAtomWithIdx(1))
              ->getIdx() == 0);
    CHECK(Chirality::getTrigonalBipyramidalAxialAtom(m->getAtomWithIdx(1), -1)
              ->getIdx() == 5);

    CHECK_THAT(
        Chirality::getIdealAngleBetweenLigands(
            m->getAtomWithIdx(1), m->getAtomWithIdx(0), m->getAtomWithIdx(2)),
        Catch::Matchers::WithinAbs(90, 0.001));
    CHECK_THAT(
        Chirality::getIdealAngleBetweenLigands(
            m->getAtomWithIdx(1), m->getAtomWithIdx(0), m->getAtomWithIdx(3)),
        Catch::Matchers::WithinAbs(90, 0.001));
    CHECK_THAT(
        Chirality::getIdealAngleBetweenLigands(
            m->getAtomWithIdx(1), m->getAtomWithIdx(0), m->getAtomWithIdx(4)),
        Catch::Matchers::WithinAbs(90, 0.001));
    CHECK_THAT(
        Chirality::getIdealAngleBetweenLigands(
            m->getAtomWithIdx(1), m->getAtomWithIdx(2), m->getAtomWithIdx(3)),
        Catch::Matchers::WithinAbs(120, 0.001));
    CHECK_THAT(
        Chirality::getIdealAngleBetweenLigands(
            m->getAtomWithIdx(1), m->getAtomWithIdx(0), m->getAtomWithIdx(5)),
        Catch::Matchers::WithinAbs(180, 0.001));
    CHECK_THAT(
        Chirality::getIdealAngleBetweenLigands(
            m->getAtomWithIdx(1), m->getAtomWithIdx(5), m->getAtomWithIdx(2)),
        Catch::Matchers::WithinAbs(90, 0.001));
    CHECK_THAT(
        Chirality::getIdealAngleBetweenLigands(
            m->getAtomWithIdx(1), m->getAtomWithIdx(5), m->getAtomWithIdx(3)),
        Catch::Matchers::WithinAbs(90, 0.001));
    CHECK_THAT(
        Chirality::getIdealAngleBetweenLigands(
            m->getAtomWithIdx(1), m->getAtomWithIdx(5), m->getAtomWithIdx(4)),
        Catch::Matchers::WithinAbs(90, 0.001));
  }
  SECTION("TB1 missing 1") {
    auto m = "S[As@TB1](F)(Cl)Br"_smiles;
    REQUIRE(m);

    CHECK(Chirality::isTrigonalBipyramidalAxialAtom(m->getAtomWithIdx(1),
                                                    m->getAtomWithIdx(0)));
    CHECK(!Chirality::isTrigonalBipyramidalAxialAtom(m->getAtomWithIdx(1),
                                                     m->getAtomWithIdx(2)));
    CHECK(!Chirality::isTrigonalBipyramidalAxialAtom(m->getAtomWithIdx(1),
                                                     m->getAtomWithIdx(3)));
    CHECK(!Chirality::isTrigonalBipyramidalAxialAtom(m->getAtomWithIdx(1),
                                                     m->getAtomWithIdx(4)));
    CHECK(Chirality::getTrigonalBipyramidalAxialAtom(m->getAtomWithIdx(1))
              ->getIdx() == 0);
    CHECK(Chirality::getTrigonalBipyramidalAxialAtom(m->getAtomWithIdx(1),
                                                     -1) == nullptr);

    CHECK_THAT(
        Chirality::getIdealAngleBetweenLigands(
            m->getAtomWithIdx(1), m->getAtomWithIdx(0), m->getAtomWithIdx(2)),
        Catch::Matchers::WithinAbs(90, 0.001));
    CHECK_THAT(
        Chirality::getIdealAngleBetweenLigands(
            m->getAtomWithIdx(1), m->getAtomWithIdx(0), m->getAtomWithIdx(3)),
        Catch::Matchers::WithinAbs(90, 0.001));
    CHECK_THAT(
        Chirality::getIdealAngleBetweenLigands(
            m->getAtomWithIdx(1), m->getAtomWithIdx(0), m->getAtomWithIdx(4)),
        Catch::Matchers::WithinAbs(90, 0.001));
    CHECK_THAT(
        Chirality::getIdealAngleBetweenLigands(
            m->getAtomWithIdx(1), m->getAtomWithIdx(2), m->getAtomWithIdx(3)),
        Catch::Matchers::WithinAbs(120, 0.001));
  }
}

TEST_CASE("getChiralPermutation", "[nontetrahedral]") {
  SECTION("TB1") {
    // clang-format off
    std::vector<std::pair<std::list<int>, unsigned int>> data = {
        {{2, 3, 4, 5, 6}, 1},
        {{2, 3, 5, 4, 6}, 2},

        {{2, 3, 4, 6, 5}, 3},
        {{2, 3, 5, 6, 4}, 4},

        {{2, 3, 6, 4, 5}, 5},
        {{2, 3, 6, 5, 4}, 6},

        {{2, 6, 3, 4, 5}, 7},
        {{2, 6, 3, 5, 4}, 8},

        {{3, 2, 4, 5, 6}, 9},
        {{3, 2, 5, 4, 6}, 11},

        {{3, 2, 4, 6, 5}, 10},
        {{3, 2, 5, 6, 4}, 12},

        {{3, 2, 6, 4, 5}, 13},
        {{3, 2, 6, 5, 4}, 14},

        {{3, 4, 2, 5, 6}, 15},
        {{3, 5, 2, 4, 6}, 20},

        {{3, 4, 2, 6, 5}, 16},
        {{3, 5, 2, 6, 4}, 19},

        {{3, 4, 5, 2, 6}, 17},
        {{3, 5, 4, 2, 6}, 18},


    };
    // clang-format on
    auto m = "CCS[As@TB1](F)(Cl)(Br)N"_smiles;
    REQUIRE(m);
    const auto atm = m->getAtomWithIdx(3);
    for (const auto &pr : data) {
      // std::cerr << "---- " << pr.second << std::endl;
      CHECK(Chirality::getChiralPermutation(atm, pr.first) == pr.second);
    }
  }

  SECTION("SP1") {
    // clang-format off
    std::vector<std::pair<std::list<int>, unsigned int>> data = {
        {{2, 3, 4, 5}, 1},
        {{2, 4, 3, 5}, 2},
        {{2, 3, 5, 4}, 3},
    };
    // clang-format on
    auto m = "CCC[Pt@SP1](F)(Cl)N"_smiles;
    REQUIRE(m);
    const auto atm = m->getAtomWithIdx(3);
    for (const auto &pr : data) {
      // std::cerr << "---- " << pr.second << std::endl;
      CHECK(Chirality::getChiralPermutation(atm, pr.first) == pr.second);
    }
  }
  SECTION("OH1") {
    // clang-format off
    std::vector<std::pair<std::list<int>, unsigned int>> data = {
        {{2, 3, 4, 5, 6, 7}, 1},
        {{2, 3, 6, 5, 4, 7}, 2},

        {{2, 3, 4, 5, 7, 6}, 3},
        {{2, 3, 6, 5, 7, 4}, 16},

        {{2, 3, 4, 7, 5, 6}, 6},
        {{2, 3, 6, 7, 5, 4}, 18},

        {{2, 3, 7, 4, 5, 6}, 19},
        {{2, 3, 7, 6, 5, 4}, 24},

        {{2, 7, 3, 4, 5, 6}, 25},
        {{2, 7, 3, 6, 5, 4}, 30},

        {{2, 3, 4, 6, 5, 7}, 4},
        {{2, 3, 6, 4, 5, 7}, 14},

        {{2, 3, 4, 6, 7, 5}, 5},
        {{2, 3, 6, 4, 7, 5}, 15},

        {{2, 3, 4, 7, 6, 5}, 7},
        {{2, 3, 6, 7, 4, 5}, 17},

        {{2, 3, 7, 4, 6, 5}, 20},
        {{2, 3, 7, 6, 4, 5}, 23},

        {{2, 7, 3, 4, 6, 5}, 26},
        {{2, 7, 3, 6, 4, 5}, 29},

        {{2, 3, 5, 6, 4, 7}, 10},
        {{2, 3, 5, 4, 6, 7}, 8},

        {{2, 3, 5, 6, 7, 4}, 11},
        {{2, 3, 5, 4, 7, 6}, 9},

        {{2, 3, 5, 7, 6, 4}, 13},
        {{2, 3, 5, 7, 4, 6}, 12},

        {{2, 3, 7, 5, 6, 4}, 22},
        {{2, 3, 7, 5, 4, 6}, 21},

        {{2, 7, 3, 5, 6, 4}, 28},
        {{2, 7, 3, 5, 4, 6}, 27},

    };
    // clang-format on
    auto m = "CCO[Co@OH1](Cl)(C)(N)(F)P"_smiles;
    REQUIRE(m);
    const auto atm = m->getAtomWithIdx(3);
    for (const auto &pr : data) {
      // std::cerr << "---- " << pr.second << std::endl;
      CHECK(Chirality::getChiralPermutation(atm, pr.first) == pr.second);
    }
  }
}

TEST_CASE("isAtomPotentialNontetrahedralCenter", "[nontetrahedral]") {
  SECTION("basics") {
    {
      auto mol = "C[S+](O)F"_smiles;
      REQUIRE(mol);
      CHECK(!Chirality::detail::isAtomPotentialNontetrahedralCenter(
          mol->getAtomWithIdx(1)));
    }
    {
      auto mol = "C[SH](O)F"_smiles;
      REQUIRE(mol);
      CHECK(Chirality::detail::isAtomPotentialNontetrahedralCenter(
          mol->getAtomWithIdx(1)));
    }
    {
      auto mol = "C[S@SP](O)F"_smiles;
      REQUIRE(mol);
      CHECK(Chirality::detail::isAtomPotentialNontetrahedralCenter(
          mol->getAtomWithIdx(1)));
    }
  }
}
TEST_CASE("nontetrahedral StereoInfo", "[nontetrahedral]") {
  SECTION("SP") {
    auto m = "C[Pt@SP1](F)(Cl)O"_smiles;
    REQUIRE(m);
    auto sinfo = Chirality::findPotentialStereo(*m);
    REQUIRE(sinfo.size() == 1);
    CHECK(sinfo[0].centeredOn == 1);
    CHECK(sinfo[0].type == Chirality::StereoType::Atom_SquarePlanar);
    CHECK(sinfo[0].descriptor == Chirality::StereoDescriptor::None);
    CHECK(sinfo[0].permutation == 1);
    CHECK(sinfo[0].controllingAtoms == std::vector<unsigned int>{0, 2, 3, 4});
  }
  SECTION("TB") {
    auto m = "C[Fe@TB4](F)(Cl)(O)N"_smiles;
    REQUIRE(m);
    auto sinfo = Chirality::findPotentialStereo(*m);
    REQUIRE(sinfo.size() == 1);
    CHECK(sinfo[0].centeredOn == 1);
    CHECK(sinfo[0].type == Chirality::StereoType::Atom_TrigonalBipyramidal);
    CHECK(sinfo[0].descriptor == Chirality::StereoDescriptor::None);
    CHECK(sinfo[0].permutation == 4);
    CHECK(sinfo[0].controllingAtoms ==
          std::vector<unsigned int>{0, 2, 3, 4, 5});
  }
  SECTION("TB0") {
    auto m = "C[Fe@TB](F)(Cl)(O)N"_smiles;
    REQUIRE(m);
    auto sinfo = Chirality::findPotentialStereo(*m);
    REQUIRE(sinfo.size() == 1);
    CHECK(sinfo[0].centeredOn == 1);
    CHECK(sinfo[0].specified == Chirality::StereoSpecified::Unknown);
    CHECK(sinfo[0].type == Chirality::StereoType::Atom_TrigonalBipyramidal);
    CHECK(sinfo[0].descriptor == Chirality::StereoDescriptor::None);
    CHECK(sinfo[0].permutation == 0);
    CHECK(sinfo[0].controllingAtoms ==
          std::vector<unsigned int>{0, 2, 3, 4, 5});
  }
  SECTION("perceived as possible") {
    auto m = "C[Fe](F)(Cl)(O)N"_smiles;
    REQUIRE(m);
    auto sinfo = Chirality::findPotentialStereo(*m);
    REQUIRE(sinfo.size() == 1);
    CHECK(sinfo[0].centeredOn == 1);
    CHECK(sinfo[0].specified == Chirality::StereoSpecified::Unspecified);
    CHECK(sinfo[0].type == Chirality::StereoType::Atom_TrigonalBipyramidal);
    CHECK(sinfo[0].descriptor == Chirality::StereoDescriptor::None);
    CHECK(sinfo[0].permutation == 0);
    CHECK(sinfo[0].controllingAtoms ==
          std::vector<unsigned int>{0, 2, 3, 4, 5});
  }

  SECTION("OH") {
    auto m = "C[Fe@OH9](F)(Cl)(O)(N)Br"_smiles;
    REQUIRE(m);
    auto sinfo = Chirality::findPotentialStereo(*m);
    REQUIRE(sinfo.size() == 1);
    CHECK(sinfo[0].centeredOn == 1);
    CHECK(sinfo[0].type == Chirality::StereoType::Atom_Octahedral);
    CHECK(sinfo[0].descriptor == Chirality::StereoDescriptor::None);
    CHECK(sinfo[0].permutation == 9);
    CHECK(sinfo[0].controllingAtoms ==
          std::vector<unsigned int>{0, 2, 3, 4, 5, 6});
  }
  SECTION("OH missing ligand") {
    auto m = "C[Fe@OH9](F)(Cl)(O)N"_smiles;
    REQUIRE(m);
    auto sinfo = Chirality::findPotentialStereo(*m);
    REQUIRE(sinfo.size() == 1);
    CHECK(sinfo[0].centeredOn == 1);
    CHECK(sinfo[0].type == Chirality::StereoType::Atom_Octahedral);
    CHECK(sinfo[0].descriptor == Chirality::StereoDescriptor::None);
    CHECK(sinfo[0].permutation == 9);
    CHECK(sinfo[0].controllingAtoms ==
          std::vector<unsigned int>{0, 2, 3, 4, 5});
  }
}

TEST_CASE("github #5328: assignChiralTypesFrom3D() ignores wiggly bonds") {
  SECTION("basics") {
    auto m = R"CTAB(
     RDKit          3D

  0  0  0  0  0  0  0  0  0  0999 V3000
M  V30 BEGIN CTAB
M  V30 COUNTS 5 4 0 0 0
M  V30 BEGIN ATOM
M  V30 1 C 0.900794 -0.086835 0.009340 0
M  V30 2 C -0.552652 0.319534 0.077502 0
M  V30 3 F -0.861497 0.413307 1.437370 0
M  V30 4 Cl -0.784572 1.925710 -0.672698 0
M  V30 5 O -1.402227 -0.583223 -0.509512 0
M  V30 END ATOM
M  V30 BEGIN BOND
M  V30 1 1 1 2
M  V30 2 1 2 3
M  V30 3 1 2 4 CFG=2
M  V30 4 1 2 5
M  V30 END BOND
M  V30 END CTAB
M  END)CTAB"_ctab;
    REQUIRE(m);
    MolOps::assignChiralTypesFrom3D(*m);
    CHECK(m->getAtomWithIdx(1)->getChiralTag() ==
          Atom::ChiralType::CHI_UNSPECIFIED);
  }
  SECTION("non-tetrahedral") {
    auto m = R"CTAB(
  Mrv2108 05252216313D

  0  0  0     0  0            999 V3000
M  V30 BEGIN CTAB
M  V30 COUNTS 6 5 0 0 0
M  V30 BEGIN ATOM
M  V30 1 C -1.7191 0.2488 -3.5085 0
M  V30 2 As -1.0558 1.9209 -2.6345 0
M  V30 3 F -0.4636 3.422 -1.7567 0
M  V30 4 O -2.808 2.4243 -2.1757 0
M  V30 5 Cl -0.1145 2.6609 -4.5048 0
M  V30 6 Br 0.2255 0.6458 -1.079 0
M  V30 END ATOM
M  V30 BEGIN BOND
M  V30 1 1 1 2
M  V30 2 1 2 3
M  V30 3 1 2 4
M  V30 4 1 2 5 CFG=2
M  V30 5 1 2 6
M  V30 END BOND
M  V30 END CTAB
M  END
)CTAB"_ctab;
    REQUIRE(m);
    MolOps::assignChiralTypesFrom3D(*m);
    CHECK(m->getAtomWithIdx(1)->getChiralTag() ==
          Atom::ChiralType::CHI_UNSPECIFIED);
  }
}

TEST_CASE("useLegacyStereoPerception feature flag") {
  SECTION("original failing example") {
    Chirality::setUseLegacyStereoPerception(true);
    auto m = "C[C@H]1CCC2(CC1)CC[C@H](C)C(C)C2"_smiles;
    REQUIRE(m);
    CHECK(m->getAtomWithIdx(1)->getChiralTag() == Atom::CHI_UNSPECIFIED);
    CHECK(m->getAtomWithIdx(9)->getChiralTag() != Atom::CHI_UNSPECIFIED);
  }
  SECTION("use new code") {
    Chirality::setUseLegacyStereoPerception(false);
    auto m = "C[C@H]1CCC2(CC1)CC[C@H](C)C(C)C2"_smiles;
    REQUIRE(m);
    CHECK(m->getAtomWithIdx(1)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    CHECK(m->getAtomWithIdx(9)->getChiralTag() != Atom::CHI_UNSPECIFIED);
  }
  std::string molblock = R"CTAB(
  Mrv2108 05202206352D          

  0  0  0     0  0            999 V3000
M  V30 BEGIN CTAB
M  V30 COUNTS 14 15 0 0 0
M  V30 BEGIN ATOM
M  V30 1 C -4.5417 3.165 0 0
M  V30 2 C -5.8753 2.395 0 0
M  V30 3 C -5.8753 0.855 0 0
M  V30 4 C -4.5417 0.085 0 0 CFG=1
M  V30 5 C -3.208 0.855 0 0
M  V30 6 C -3.208 2.395 0 0
M  V30 7 C -4.5417 -1.455 0 0
M  V30 8 C -1.8743 0.085 0 0
M  V30 9 C -4.5417 6.2451 0 0 CFG=2
M  V30 10 C -5.8753 5.4751 0 0
M  V30 11 C -5.8753 3.9351 0 0
M  V30 12 C -3.208 3.9351 0 0
M  V30 13 C -3.208 5.4751 0 0
M  V30 14 C -4.5417 7.7851 0 0
M  V30 END ATOM
M  V30 BEGIN BOND
M  V30 1 1 1 2
M  V30 2 1 2 3
M  V30 3 1 3 4
M  V30 4 1 4 5
M  V30 5 1 5 6
M  V30 6 1 1 6
M  V30 7 1 4 7 CFG=1
M  V30 8 1 5 8
M  V30 9 1 9 10
M  V30 10 1 10 11
M  V30 11 1 12 13
M  V30 12 1 9 13
M  V30 13 1 11 1
M  V30 14 1 1 12
M  V30 15 1 9 14 CFG=1
M  V30 END BOND
M  V30 END CTAB
M  END
)CTAB";
  SECTION("original example, from mol block") {
    Chirality::setUseLegacyStereoPerception(true);
    std::unique_ptr<RWMol> m{MolBlockToMol(molblock)};
    CHECK(m->getAtomWithIdx(3)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    CHECK(m->getAtomWithIdx(8)->getChiralTag() == Atom::CHI_UNSPECIFIED);
  }
  SECTION("original example, from mol block, new code") {
    Chirality::setUseLegacyStereoPerception(false);
    std::unique_ptr<RWMol> m{MolBlockToMol(molblock)};
    CHECK(m->getAtomWithIdx(3)->getChiralTag() != Atom::CHI_UNSPECIFIED);
    CHECK(m->getAtomWithIdx(8)->getChiralTag() != Atom::CHI_UNSPECIFIED);
  }
}

TEST_CASE("wedgeMolBonds to aromatic rings") {
  auto m = R"CTAB(
     RDKit          2D

  0  0  0  0  0  0  0  0  0  0999 V3000
M  V30 BEGIN CTAB
M  V30 COUNTS 10 11 0 0 0
M  V30 BEGIN ATOM
M  V30 1 C 2.948889 -2.986305 0.000000 0
M  V30 2 C 2.560660 -4.435194 0.000000 0
M  V30 3 N 1.111771 -4.046965 0.000000 0
M  V30 4 C 1.500000 -2.598076 0.000000 0
M  V30 5 C 0.750000 -1.299038 0.000000 0
M  V30 6 C 1.500000 0.000000 0.000000 0
M  V30 7 C 0.750000 1.299038 0.000000 0
M  V30 8 C -0.750000 1.299038 0.000000 0
M  V30 9 C -1.500000 0.000000 0.000000 0
M  V30 10 C -0.750000 -1.299038 0.000000 0
M  V30 END ATOM
M  V30 BEGIN BOND
M  V30 1 1 1 2
M  V30 2 1 2 3
M  V30 3 1 3 4 
M  V30 4 1 4 5 CFG=1
M  V30 5 2 5 6
M  V30 6 1 6 7
M  V30 7 2 7 8
M  V30 8 1 8 9
M  V30 9 2 9 10
M  V30 10 1 4 1
M  V30 11 1 10 5
M  V30 END BOND
M  V30 END CTAB
M  END
)CTAB"_ctab;
  REQUIRE(m);
  CHECK(m->getAtomWithIdx(3)->getChiralTag() !=
        Atom::ChiralType::CHI_UNSPECIFIED);
  CHECK(m->getBondWithIdx(2)->getBondDir() == Bond::BondDir::NONE);
  CHECK(m->getBondWithIdx(3)->getBondDir() == Bond::BondDir::NONE);

  SECTION("generating mol blocks") {
    auto mb = MolToV3KMolBlock(*m);
    CHECK(mb.find("M  V30 10 1 4 1 CFG=1") == std::string::npos);
    CHECK(mb.find("M  V30 4 1 4 5 CFG=1") != std::string::npos);
  }

  SECTION("details: pickBondsWedge()") {
    // this is with aromatic bonds
    auto bnds = pickBondsToWedge(*m);
    CHECK(bnds.at(3) == 3);
    RWMol cp(*m);

    // now try kekulized:
    MolOps::Kekulize(cp);
    bnds = pickBondsToWedge(cp);
    CHECK(bnds.at(3) == 3);
  }
}

TEST_CASE("github 5307: AssignAtomChiralTagsFromStructure ignores Hydrogens") {
  std::string mb = R"CTAB(
     RDKit          3D
     
  0  0  0  0  0  0  0  0  0  0999 V3000
M  V30 BEGIN CTAB
M  V30 COUNTS 5 4 0 0 0
M  V30 BEGIN ATOM
M  V30 1 C -0.022097 0.003215 0.016520 0
M  V30 2 H -0.669009 0.889360 -0.100909 0
M  V30 3 H -0.377788 -0.857752 -0.588296 0
M  V30 4 H 0.096421 -0.315125 1.063781 0
M  V30 5 H 0.972473 0.280302 -0.391096 0
M  V30 END ATOM
M  V30 BEGIN BOND
M  V30 1 1 1 2
M  V30 2 1 1 3
M  V30 3 1 1 4
M  V30 4 1 1 5
M  V30 END BOND
M  V30 END CTAB
M  END
)CTAB";
  bool sanitize = true;
  bool removeHs = false;
  std::unique_ptr<RWMol> m{MolBlockToMol(mb, sanitize, removeHs)};
  REQUIRE(m);
  MolOps::assignChiralTypesFrom3D(*m);
  CHECK(m->getAtomWithIdx(0)->getChiralTag() !=
        Atom::ChiralType::CHI_UNSPECIFIED);
  // assignStereochemistryFrom3D() actually checks:
  MolOps::assignStereochemistryFrom3D(*m);
  CHECK(m->getAtomWithIdx(0)->getChiralTag() ==
        Atom::ChiralType::CHI_UNSPECIFIED);
}
