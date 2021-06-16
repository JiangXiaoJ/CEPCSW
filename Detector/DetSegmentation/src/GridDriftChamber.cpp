#include "DetSegmentation/GridDriftChamber.h"
#include <map>

namespace dd4hep {
namespace DDSegmentation {

/// default constructor using an encoding string
GridDriftChamber::GridDriftChamber(const std::string& cellEncoding) : Segmentation(cellEncoding) {
  // define type and description
  _type = "GridDriftChamber";
  _description = "Drift chamber segmentation in the global coordinates";

  registerParameter("cell_size", "cell size", m_cellSize, 0., SegmentationParameter::LengthUnit);
  registerParameter("detector_length", "Length of the wire", m_detectorLength, 1., SegmentationParameter::LengthUnit);
  registerIdentifier("identifier_phi", "Cell ID identifier for phi", m_phiID, "cellID");
  registerIdentifier("layerID", "layer id", layer_id, "layer");
}

GridDriftChamber::GridDriftChamber(const BitFieldCoder* decoder) : Segmentation(decoder) {

  _type = "GridDriftChamber";
  _description = "Drift chamber segmentation in the global coordinates";

  registerParameter("cell_size", "cell size", m_cellSize, 1., SegmentationParameter::LengthUnit);
  registerParameter("epsilon0", "epsilon", m_epsilon0, 0., SegmentationParameter::AngleUnit, true);
  registerParameter("detector_length", "Length of the wire", m_detectorLength, 1., SegmentationParameter::LengthUnit);
  registerIdentifier("identifier_phi", "Cell ID identifier for phi", m_phiID, "cellID");
  registerIdentifier("layerID", "layer id", layer_id, "layer");
  registerParameter("safe_distance", "safe_distance", m_safe_distance, 0., SegmentationParameter::LengthUnit);
  registerParameter("layer_width", "layer_width", m_layer_width, 0., SegmentationParameter::LengthUnit);
  registerParameter("DC_rbegin", "DC_rbegin", m_DC_rbegin, 0., SegmentationParameter::LengthUnit);
  registerParameter("DC_rend", "DC_rend", m_DC_rend, 0., SegmentationParameter::LengthUnit);
  registerParameter("DC_rmin", "DC_rmin", m_DC_rmin, 0., SegmentationParameter::LengthUnit);
  registerParameter("DC_rmax", "DC_rmax", m_DC_rmax, 0., SegmentationParameter::LengthUnit);
}

Vector3D GridDriftChamber::position(const CellID& /*cID*/) const {
  Vector3D cellPosition = {0, 0, 0};
  return cellPosition;
}

CellID GridDriftChamber::cellID(const Vector3D& /*localPosition*/, const Vector3D& globalPosition,
                                const VolumeID& vID) const {

  CellID cID = vID;

  int chamberID = _decoder->get(cID, "chamber");

  double posx = globalPosition.X;
  double posy = globalPosition.Y;
  double radius = sqrt(posx*posx+posy*posy);

  int m_DC_layer_number = floor((m_DC_rend-m_DC_rbegin)/m_layer_width);
  double DC_layerdelta = m_layer_width;

  int layerid;
  if( radius<= m_DC_rend && radius>= m_DC_rbegin) {
      layerid = floor((radius - m_DC_rbegin)/DC_layerdelta);
  } else if ( radius>= (m_DC_rmin-m_safe_distance) && radius < m_DC_rbegin) {
      layerid = 0;
  } else if ( radius> m_DC_rend && radius <= (m_DC_rmax+m_safe_distance)) {
      layerid = m_DC_layer_number-1;
  }

  updateParams(chamberID,layerid);

  double phi_hit = phiFromXY(globalPosition);
  double offsetphi= m_offset;
  int _lphi;

  if(phi_hit >= offsetphi) {
    _lphi = (int) ((phi_hit - offsetphi)/ _currentLayerphi);
  }
  else {
    _lphi = (int) ((phi_hit - offsetphi + 2 * M_PI)/ _currentLayerphi);
  }

  int lphi = _lphi;
  _decoder->set(cID, layer_id, layerid);
  _decoder->set(cID, m_phiID, lphi);

  return cID;
}

double GridDriftChamber::phi(const CellID& cID) const {
  CellID phiValue = _decoder->get(cID, m_phiID);
  return binToPosition(phiValue, _currentLayerphi, m_offset);
}

void GridDriftChamber::cellposition(const CellID& cID, TVector3& Wstart,
                                    TVector3& Wend) const {

  auto chamberIndex = _decoder->get(cID, "chamber");
  auto layerIndex = _decoder->get(cID, "layer");
  updateParams(chamberIndex,layerIndex);

  double phi_start = phi(cID);
  double phi_mid = phi_start + _currentLayerphi/2.;
  double phi_end = phi_mid + returnAlpha();

  Wstart = returnWirePosition(phi_mid, -1);
  Wend = returnWirePosition(phi_end, 1);
}

double GridDriftChamber::distanceTrackWire(const CellID& cID, const TVector3& hit_start,
                                           const TVector3& hit_end) const {

  TVector3 Wstart = {0,0,0};
  TVector3 Wend = {0,0,0};
  cellposition(cID,Wstart,Wend);

  TVector3 a = (hit_end - hit_start).Unit();
  TVector3 b = (Wend - Wstart).Unit();
  TVector3 c = Wstart - hit_start;

  double num = std::abs(c.Dot(a.Cross(b)));
  double denum = (a.Cross(b)).Mag();

  double DCA = 0;

  if (denum) {
      DCA = num / denum;
  }

  return DCA;
}

double GridDriftChamber::distanceTrackWire2(const CellID& cID, const TVector3& hit_pos) const {

    TVector3 Wstart = {0,0,0};
    TVector3 Wend = {0,0,0};
    cellposition(cID,Wstart,Wend);

    TVector3 denominator = Wend - Wstart;
    TVector3  numerator = denominator.Cross(Wstart-hit_pos);

    double DCA = numerator.Mag()/denominator.Mag() ;

    return DCA;
}

TVector3 GridDriftChamber::distanceClosestApproach(const CellID& cID, const TVector3& hitPos) const {
  // Distance of the closest approach between a single hit (point) and the closest wire

   TVector3 Wstart = {0,0,0};
   TVector3 Wend = {0,0,0};
   cellposition(cID,Wstart,Wend);

   TVector3 temp = (Wend + Wstart);
   TVector3 Wmid(temp.X() / 2.0, temp.Y() / 2.0, temp.Z() / 2.0);

   double hitPhi = hitPos.Phi();
   if (hitPhi < 0) {
       hitPhi = hitPhi + 2 * M_PI;
   }

   TVector3 PCA = Wstart + ((Wend - Wstart).Unit()).Dot((hitPos - Wstart)) * ((Wend - Wstart).Unit());
   TVector3 dca = hitPos - PCA;

   return dca;
}

TVector3 GridDriftChamber::Line_TrackWire(const CellID& cID, const TVector3& hit_start, const TVector3& hit_end) const {
  // The line connecting a particle track to the closest wire
  // Returns the vector connecting the both
  TVector3 Wstart = {0,0,0};
  TVector3 Wend = {0,0,0};
  cellposition(cID,Wstart,Wend);

  TVector3 P1 = hit_start;
  TVector3 P2 = hit_end;
  TVector3 P3 = Wstart;
  TVector3 P4 = Wend;

  TVector3 intersect = LineLineIntersect(P1, P2, P3, P4);
  return intersect;
}

// Get the wire position for a z
TVector3 GridDriftChamber::wirePos_vs_z(const CellID& cID, const double& zpos) const {

  TVector3 Wstart = {0,0,0};
  TVector3 Wend = {0,0,0};
  cellposition(cID,Wstart,Wend);

  double t = (zpos - Wstart.Z())/(Wend.Z()-Wstart.Z());
  double x = Wstart.X()+t*(Wend.X()-Wstart.X());
  double y = Wstart.Y()+t*(Wend.Y()-Wstart.Y());

  TVector3 wireCoord(x, y, zpos);
  return wireCoord;
}

TVector3 GridDriftChamber::IntersectionTrackWire(const CellID& cID, const TVector3& hit_start, const TVector3& hit_end) const {
  // Intersection between the particle track and the wire assuming that the track between hit_start and hit_end is linear

  TVector3 Wstart = {0,0,0};
  TVector3 Wend = {0,0,0};
  cellposition(cID,Wstart,Wend);

  TVector3 P1 = hit_start;
  TVector3 V1 = hit_end-hit_start;

  TVector3 P2 = Wstart;
  TVector3 V2 = Wend - Wstart;

  TVector3 denom = V1.Cross(V2);
  double mag_denom = denom.Mag();

  TVector3 intersect(0, 0, 0);

  if (mag_denom !=0)
    {
      TVector3 num = ((P2-P1)).Cross(V2);
      double mag_num = num.Mag();
      double a = mag_num / mag_denom;

      intersect = P1 + a * V1;

    }
  return intersect;
}


}
}
