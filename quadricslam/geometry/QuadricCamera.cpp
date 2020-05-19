/* ----------------------------------------------------------------------------

 * QuadricSLAM Copyright 2020, ARC Centre of Excellence for Robotic Vision, Queensland University of Technology (QUT)
 * Brisbane, QLD 4000
 * All Rights Reserved
 * Authors: Lachlan Nicholson, et al. (see THANKS for the full author list)
 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file QuadricCamera.cpp
 * @date Apr 16, 2020
 * @author Lachlan Nicholson
 * @brief a class responsible for projecting quadrics
 */

#include <quadricslam/geometry/QuadricCamera.h>
#include <quadricslam/base/Jacobians.h>
#include <quadricslam/geometry/BoundingBoxFactor.h> // for CHECK_ANALYTICAL

#include <gtsam/base/numericalDerivative.h>

namespace gtsam {

/* ************************************************************************* */
QuadricCamera QuadricCamera::Create(const Pose3& pose, const boost::shared_ptr<Cal3_S2>& K, OptionalJacobian<6,6> dCamera_dPose, OptionalJacobian<6,5> dCamera_dCalibration) {
  if (dCamera_dPose) {
    *dCamera_dPose = Matrix66::Identity();
  } if (dCamera_dCalibration) {
    *dCamera_dCalibration = Matrix65::Zero();
  }
  return QuadricCamera(pose, K);
}

/* ************************************************************************* */
// Note: will compute inverse camPose with jacobian regardless of OptionalJacobian
Matrix34 QuadricCamera::transformToImage(OptionalJacobian<12,6> dP_dCamera) const {
  
  Matrix3 image_T_camera = calibration().K();
  Matrix4 camera_T_world = pose().inverse().matrix();
  Matrix34 image_T_world = image_T_camera * (camera_T_world).block(0,0,3,4);
  
  if (dP_dCamera) {
    throw NotImplementedException();
    // throw std::runtime_error("Rot3::CayleyChart::Local Derivative");
  }
  return image_T_world;
}

/* ************************************************************************* */
DualConic QuadricCamera::project(const ConstrainedDualQuadric& quadric) const {
  Matrix34 image_T_world = transformToImage(); 
  Matrix4 dQ = quadric.matrix();
  // dQ = dQ/dQ(3,3);
  Matrix3 dC = image_T_world * dQ * image_T_world.transpose();
  return DualConic(dC);
}

/* ************************************************************************* */
Matrix3 QuadricCamera::project_(const ConstrainedDualQuadric& quadric, const Pose3& pose, const boost::shared_ptr<Cal3_S2>& calibration) {
  Matrix3 C = project(quadric, pose, calibration).matrix();
  return C;
}

/* ************************************************************************* */
Matrix34 QuadricCamera::transformToImage(const Pose3& pose, const boost::shared_ptr<Cal3_S2>& calibration) {
  Matrix3 image_T_camera = calibration->K();
  Matrix4 camera_T_world = pose.inverse().matrix();
  Matrix34 image_T_world = image_T_camera * (camera_T_world).block(0,0,3,4);
  // Matrix34 image_T_world = image_T_camera * internal::I34 * camera_T_world;
  return image_T_world;
}

/* ************************************************************************* */
// NOTE: requires updating jacobians if we normalize q/c
// this wont happen if we split it into sub functions and just combine jacobians
DualConic QuadricCamera::project(const ConstrainedDualQuadric& quadric, const Pose3& pose, const boost::shared_ptr<Cal3_S2>& calibration, 
  OptionalJacobian<9,9> dc_dq, OptionalJacobian<9,6> dc_dx, OptionalJacobian<9,5> dc_dk) {
  
  using namespace internal;
  using namespace std;

  // first retract quadric and pose to compute dX:/dx and dQ:/dq
  Matrix3 K = calibration->K();
  Matrix4 Xi = pose.inverse().matrix();
  Matrix34 P = K * I34 * Xi;
  Matrix4 Q = quadric.matrix();
  Matrix3 C = P * Q * P.transpose();
  DualConic dualConic(C);

  if (dc_dq) {
    Eigen::Matrix<double, 9,16> dC_dQ = kron(P, P);
    Eigen::Matrix<double, 16,9> dQ_dq; quadric.matrix(dQ_dq); // NOTE: this recalculates quadric.matrix
    *dc_dq = dC_dQ * dQ_dq;

    if (CHECK_ANALYTICAL) {

      // we want to derive wrt the conic matrix 
      auto project_funptr = [&](const ConstrainedDualQuadric& q, const Pose3& x) -> Matrix33 { return QuadricCamera::project(q, x, calibration).matrix();};

      // cast to boost::function for numericalDerivative
      auto boost_funptr(static_cast<boost::function<Matrix3(const ConstrainedDualQuadric&,  const Pose3&)>>(project_funptr));

      // calculate derivative of conic_matrix wrt quadric vector
      Eigen::Matrix<double, 9,9> dC_dq_ = numericalDerivative21(boost_funptr, quadric, pose, 1e-6);

      // get function pointer to quadric.matrix() with no optional jacobians
      boost::function<Matrix4(const ConstrainedDualQuadric&)> matrix_funptr(boost::bind(&ConstrainedDualQuadric::matrix, _1, boost::none));
      
      // calculate derivative of quadric matrix wrt quadric vector
      Eigen::Matrix<double, 16,9> dQ_dq_ = numericalDerivative11(matrix_funptr, quadric, 1e-6);

      // confirm analytical == numerical
      if (!dC_dq_.isApprox(*dc_dq, 1e-06)) {
        cout << "WARNING(quadriccamera/dc_dq): numerical != analytical" << endl;
        cout << "Analytical dc_dq:\n" << *dc_dq << endl;
        cout << "Numerical dc_dq:\n" << dC_dq_ << endl << endl;

        cout << "Analytical dQ_dq_:\n" << dQ_dq << endl;
        cout << "Numerical dQ_dq:\n" << dQ_dq_ << endl << endl;
      }
    }
  } 
    
  if (dc_dx) {
    Eigen::Matrix<double, 9,12> dC_dP = kron(I33, P*Q) * TVEC(3,4) + kron(P*Q.transpose(), I33);
    Eigen::Matrix<double, 12,16> dP_dXi = kron(I44, K*I34);
    Eigen::Matrix<double, 16,16> dXi_dX = -kron(Xi.transpose(), Xi);
    Eigen::Matrix<double, 16,6> dX_dx; internal::matrix(pose, dX_dx);
    *dc_dx = dC_dP * dP_dXi * dXi_dX * dX_dx;

    if (CHECK_ANALYTICAL) {

      // we want to derive wrt the conic matrix 
      auto project_funptr = [&](const ConstrainedDualQuadric& q, const Pose3& x) -> Matrix33 { return QuadricCamera::project(q, x, calibration).matrix();};

      // cast to boost::function for numericalDerivative
      auto boost_funptr(static_cast<boost::function<Matrix3(const ConstrainedDualQuadric&,  const Pose3&)>>(project_funptr));

      // calculate derivative of conic_matrix wrt pose vector
      Eigen::Matrix<double, 9,6> dC_dx_ = numericalDerivative22(boost_funptr, quadric, pose, 1e-6);

      // get function pointer to pose.matrix() with no jacobians
      boost::function<Matrix4(const Pose3&)> matrix_funptr(boost::bind(&internal::matrix, _1, boost::none));

      // calculate derivative of pose_matrix with respect to pose_vector
      Eigen::Matrix<double, 16,6> dX_dx_ = numericalDerivative11(matrix_funptr, pose, 1e-6);

      // confirm analytical == numerical
      if (!dC_dx_.isApprox(*dc_dx, 1e-06)) {
        cout << "WARNING(quadriccamera/dc_dx): numerical != analytical" << endl;
        cout << "Analytical dc_dx:\n" << *dc_dx << endl;
        cout << "Numerical dc_dx:\n" << dC_dx_ << endl << endl;

        cout << "Analytical dX_dx:\n" << dX_dx << endl;
        cout << "Numerical dX_dx:\n" << dX_dx_ << endl << endl;
      }
    }
  }
  
  if (dc_dk) {
    *dc_dk = Matrix::Zero(9,5);
  }

  return dualConic;
}

/* ************************************************************************* */
std::vector<Vector4> QuadricCamera::project(const AlignedBox2& box, const Pose3& pose, const boost::shared_ptr<Cal3_S2>& calibration) {
  std::vector<Vector4> planes;
  for (auto line : box.lines()) {
    Vector4 plane = QuadricCamera::transformToImage(pose, calibration).transpose() * line;
    planes.push_back(plane);
  }
  return planes;
}


} // namespace gtsam
