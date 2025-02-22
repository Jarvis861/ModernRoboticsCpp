#include "../include/modern_robotics.h"

/*
 * modernRobotics.cpp
 * Adapted from modern_robotics.py provided by modernrobotics.org
 * Provides useful Jacobian and frame representation functions
 */
#include <Eigen/Dense>
#include <cmath>
#include <vector>

# define M_PI           3.14159265358979323846  /* pi */

namespace mr {

	/* Function: Find if the value is negligible enough to consider 0
	 * Inputs: value to be checked as a float
	 * Returns: Boolean of true-ignore or false-can't ignore
	 */
	bool NearZero(const float val) {
		return (std::abs(val) < .000001);
	}

	/*
	 * Function: Calculate the 6x6 matrix [adV] of the given 6-vector
	 * Input: Eigen::VectorXf (6x1)
	 * Output: Eigen::MatrixXf (6x6)
	 * Note: Can be used to calculate the Lie bracket [V1, V2] = [adV1]V2
	 */
	Eigen::MatrixXf ad(Eigen::VectorXf V) {
		Eigen::Matrix3f omgmat = VecToso3(Eigen::Vector3f(V(0), V(1), V(2)));

		Eigen::MatrixXf result(6, 6);
		result.topLeftCorner<3, 3>() = omgmat;
		result.topRightCorner<3, 3>() = Eigen::Matrix3f::Zero(3, 3);
		result.bottomLeftCorner<3, 3>() = VecToso3(Eigen::Vector3f(V(3), V(4), V(5)));
		result.bottomRightCorner<3, 3>() = omgmat;
		return result;
	}

	/* Function: Returns a normalized version of the input vector
	 * Input: Eigen::MatrixXf
	 * Output: Eigen::MatrixXf
	 * Note: MatrixXf is used instead of VectorXf for the case of row vectors
	 * 		Requires a copy
	 *		Useful because of the MatrixXf casting
	 */
	Eigen::MatrixXf Normalize(Eigen::MatrixXf V) {
		V.normalize();
		return V;
	}


	/* Function: Returns the skew symmetric matrix representation of an angular velocity vector
	 * Input: Eigen::Vector3f 3x1 angular velocity vector
	 * Returns: Eigen::MatrixXf 3x3 skew symmetric matrix
	 */
	Eigen::Matrix3f VecToso3(const Eigen::Vector3f& omg) {
		Eigen::Matrix3f m_ret;
		m_ret << 0, -omg(2), omg(1),
			omg(2), 0, -omg(0),
			-omg(1), omg(0), 0;
		return m_ret;
	}


	/* Function: Returns angular velocity vector represented by the skew symmetric matrix
	 * Inputs: Eigen::MatrixXf 3x3 skew symmetric matrix
	 * Returns: Eigen::Vector3f 3x1 angular velocity
	 */
	Eigen::Vector3f so3ToVec(const Eigen::MatrixXf& so3mat) {
		Eigen::Vector3f v_ret;
		v_ret << so3mat(2, 1), so3mat(0, 2), so3mat(1, 0);
		return v_ret;
	}


	/* Function: Translates an exponential rotation into it's individual components
	 * Inputs: Exponential rotation (rotation matrix in terms of a rotation axis
	 *				and the angle of rotation)
	 * Returns: The axis and angle of rotation as [x, y, z, theta]
	 */
	Eigen::Vector4f AxisAng3(const Eigen::Vector3f& expc3) {
		Eigen::Vector4f v_ret;
		v_ret << Normalize(expc3), expc3.norm();
		return v_ret;
	}


	/* Function: Translates an exponential rotation into a rotation matrix
	 * Inputs: exponenential representation of a rotation
	 * Returns: Rotation matrix
	 */
	Eigen::Matrix3f MatrixExp3(const Eigen::Matrix3f& so3mat) {
		Eigen::Vector3f omgtheta = so3ToVec(so3mat);

		Eigen::Matrix3f m_ret = Eigen::Matrix3f::Identity();
		if (NearZero(so3mat.norm())) {
			return m_ret;
		}
		else {
			float theta = (AxisAng3(omgtheta))(3);
			Eigen::Matrix3f omgmat = so3mat * (1 / theta);
			return m_ret + std::sin(theta) * omgmat + ((1 - std::cos(theta)) * (omgmat * omgmat));
		}
	}


	/* Function: Computes the matrix logarithm of a rotation matrix
	 * Inputs: Rotation matrix
	 * Returns: matrix logarithm of a rotation
	 */
	Eigen::Matrix3f MatrixLog3(const Eigen::Matrix3f& R) {
		float acosinput = (R.trace() - 1) / 2.0;
		Eigen::MatrixXf m_ret = Eigen::MatrixXf::Zero(3, 3);
		if (acosinput >= 1)
			return m_ret;
		else if (acosinput <= -1) {
			Eigen::Vector3f omg;
			if (!NearZero(1 + R(2, 2)))
				omg = (1.0 / std::sqrt(2 * (1 + R(2, 2))))*Eigen::Vector3f(R(0, 2), R(1, 2), 1 + R(2, 2));
			else if (!NearZero(1 + R(1, 1)))
				omg = (1.0 / std::sqrt(2 * (1 + R(1, 1))))*Eigen::Vector3f(R(0, 1), 1 + R(1, 1), R(2, 1));
			else
				omg = (1.0 / std::sqrt(2 * (1 + R(0, 0))))*Eigen::Vector3f(1 + R(0, 0), R(1, 0), R(2, 0));
			m_ret = VecToso3(M_PI * omg);
			return m_ret;
		}
		else {
			float theta = std::acos(acosinput);
			m_ret = theta / 2.0 / sin(theta)*(R - R.transpose());
			return m_ret;
		}
	}

	/* Function: Combines a rotation matrix and position vector into a single
	 * 				Special Euclidian Group (SE3) homogeneous transformation matrix
	 * Inputs: Rotation Matrix (R), Position Vector (p)
	 * Returns: Matrix of T = [ [R, p],
	 *						    [0, 1] ]
	 */
	Eigen::MatrixXf RpToTrans(const Eigen::Matrix3f& R, const Eigen::Vector3f& p) {
		Eigen::MatrixXf m_ret(4, 4);
		m_ret << R, p,
			0, 0, 0, 1;
		return m_ret;
	}


	/* Function: Separates the rotation matrix and position vector from
	 *				the transfomation matrix representation
	 * Inputs: Homogeneous transformation matrix
	 * Returns: std::vector of [rotation matrix, position vector]
	 */
	std::vector<Eigen::MatrixXf> TransToRp(const Eigen::MatrixXf& T) {
		std::vector<Eigen::MatrixXf> Rp_ret;
		Eigen::Matrix3f R_ret;
		// Get top left 3x3 corner
		R_ret = T.block<3, 3>(0, 0);

		Eigen::Vector3f p_ret(T(0, 3), T(1, 3), T(2, 3));

		Rp_ret.push_back(R_ret);
		Rp_ret.push_back(p_ret);

		return Rp_ret;
	}


	/* Function: Translates a spatial velocity vector into a transformation matrix
	 * Inputs: Spatial velocity vector [angular velocity, linear velocity]
	 * Returns: Transformation matrix
	 */
	Eigen::MatrixXf VecTose3(const Eigen::VectorXf& V) {
		// Separate angular (exponential representation) and linear velocities
		Eigen::Vector3f exp(V(0), V(1), V(2));
		Eigen::Vector3f linear(V(3), V(4), V(5));

		// Fill in values to the appropriate parts of the transformation matrix
		Eigen::MatrixXf m_ret(4, 4);
		m_ret << VecToso3(exp), linear,
			0, 0, 0, 0;

		return m_ret;
	}


	/* Function: Translates a transformation matrix into a spatial velocity vector
	 * Inputs: Transformation matrix
	 * Returns: Spatial velocity vector [angular velocity, linear velocity]
	 */
	Eigen::VectorXf se3ToVec(const Eigen::MatrixXf& T) {
		Eigen::VectorXf m_ret(6);
		m_ret << T(2, 1), T(0, 2), T(1, 0), T(0, 3), T(1, 3), T(2, 3);

		return m_ret;
	}


	/* Function: Provides the adjoint representation of a transformation matrix
	 *			 Used to change the frame of reference for spatial velocity vectors
	 * Inputs: 4x4 Transformation matrix SE(3)
	 * Returns: 6x6 Adjoint Representation of the matrix
	 */
	Eigen::MatrixXf Adjoint(const Eigen::MatrixXf& T) {
		std::vector<Eigen::MatrixXf> R = TransToRp(T);
		Eigen::MatrixXf ad_ret(6, 6);
		ad_ret = Eigen::MatrixXf::Zero(6, 6);
		Eigen::MatrixXf zeroes = Eigen::MatrixXf::Zero(3, 3);
		ad_ret << R[0], zeroes,
			VecToso3(R[1]) * R[0], R[0];
		return ad_ret;
	}


	/* Function: Rotation expanded for screw axis
	 * Inputs: se3 matrix representation of exponential coordinates (transformation matrix)
	 * Returns: 6x6 Matrix representing the rotation
	 */
	Eigen::MatrixXf MatrixExp6(const Eigen::MatrixXf& se3mat) {
		// Extract the angular velocity vector from the transformation matrix
		Eigen::Matrix3f se3mat_cut = se3mat.block<3, 3>(0, 0);
		Eigen::Vector3f omgtheta = so3ToVec(se3mat_cut);

		Eigen::MatrixXf m_ret(4, 4);

		// If negligible rotation, m_Ret = [[Identity, angular velocty ]]
		//									[	0	 ,		1		   ]]
		if (NearZero(omgtheta.norm())) {
			// Reuse previous variables that have our required size
			se3mat_cut = Eigen::MatrixXf::Identity(3, 3);
			omgtheta << se3mat(0, 3), se3mat(1, 3), se3mat(2, 3);
			m_ret << se3mat_cut, omgtheta,
				0, 0, 0, 1;
			return m_ret;
		}
		// If not negligible, MR page 105
		else {
			float theta = (AxisAng3(omgtheta))(3);
			Eigen::Matrix3f omgmat = se3mat.block<3, 3>(0, 0) / theta;
			Eigen::Matrix3f expExpand = Eigen::MatrixXf::Identity(3, 3) * theta + (1 - std::cos(theta)) * omgmat + ((theta - std::sin(theta)) * (omgmat * omgmat));
			Eigen::Vector3f linear(se3mat(0, 3), se3mat(1, 3), se3mat(2, 3));
			Eigen::Vector3f GThetaV = (expExpand*linear) / theta;
			m_ret << MatrixExp3(se3mat_cut), GThetaV,
				0, 0, 0, 1;
			return m_ret;
		}

	}

	Eigen::MatrixXf MatrixLog6(const Eigen::MatrixXf& T) {
		Eigen::MatrixXf m_ret(4, 4);
		auto rp = mr::TransToRp(T);
		Eigen::Matrix3f omgmat = MatrixLog3(rp.at(0));
		Eigen::Matrix3f zeros3f = Eigen::Matrix3f::Zero(3, 3);
		if (NearZero(omgmat.norm())) {
			m_ret << zeros3f, rp.at(1),
				0, 0, 0, 0;
		}
		else {
			float theta = std::acos((rp.at(0).trace() - 1) / 2.0);
			Eigen::Matrix3f logExpand1 = Eigen::MatrixXf::Identity(3, 3) - omgmat / 2.0;
			Eigen::Matrix3f logExpand2 = (1.0 / theta - 1.0 / std::tan(theta / 2.0) / 2)*omgmat*omgmat / theta;
			Eigen::Matrix3f logExpand = logExpand1 + logExpand2;
			m_ret << omgmat, logExpand*rp.at(1),
				0, 0, 0, 0;
		}
		return m_ret;
	}


	/* Function: Compute end effector frame (used for current spatial position calculation)
	 * Inputs: Home configuration (position and orientation) of end-effector
	 *		   The joint screw axes in the space frame when the manipulator
	 *             is at the home position
	 * 		   A list of joint coordinates.
	 * Returns: Transfomation matrix representing the end-effector frame when the joints are
	 *				at the specified coordinates
	 * Notes: FK means Forward Kinematics
	 */
	Eigen::MatrixXf FKinSpace(const Eigen::MatrixXf& M, const Eigen::MatrixXf& Slist, const Eigen::VectorXf& thetaList) {
		Eigen::MatrixXf T = M;
		for (int i = (thetaList.size() - 1); i > -1; i--) {
			T = MatrixExp6(VecTose3(Slist.col(i)*thetaList(i))) * T;
		}
		return T;
	}

	/*
	 * Function: Compute end effector frame (used for current body position calculation)
	 * Inputs: Home configuration (position and orientation) of end-effector
	 *		   The joint screw axes in the body frame when the manipulator
	 *             is at the home position
	 * 		   A list of joint coordinates.
	 * Returns: Transfomation matrix representing the end-effector frame when the joints are
	 *				at the specified coordinates
	 * Notes: FK means Forward Kinematics
	 */
	Eigen::MatrixXf FKinBody(const Eigen::MatrixXf& M, const Eigen::MatrixXf& Blist, const Eigen::VectorXf& thetaList) {
		Eigen::MatrixXf T = M;
		for (int i = 0; i < thetaList.size(); i++) {
			T = T * MatrixExp6(VecTose3(Blist.col(i)*thetaList(i)));
		}
		return T;
	}


	/* Function: Gives the space Jacobian
	 * Inputs: Screw axis in home position, joint configuration
	 * Returns: 6xn Spatial Jacobian
	 */
	Eigen::MatrixXf JacobianSpace(const Eigen::MatrixXf& Slist, const Eigen::MatrixXf& thetaList) {
		Eigen::MatrixXf Js = Slist;
		Eigen::MatrixXf T = Eigen::MatrixXf::Identity(4, 4);
		Eigen::VectorXf sListTemp(Slist.col(0).size());
		for (int i = 1; i < thetaList.size(); i++) {
			sListTemp << Slist.col(i - 1) * thetaList(i - 1);
			T = T * MatrixExp6(VecTose3(sListTemp));
			// std::cout << "array: " << sListTemp << std::endl;
			Js.col(i) = Adjoint(T) * Slist.col(i);
		}

		return Js;
	}

	/*
	 * Function: Gives the body Jacobian
	 * Inputs: Screw axis in BODY position, joint configuration
	 * Returns: 6xn Bobdy Jacobian
	 */
	Eigen::MatrixXf JacobianBody(const Eigen::MatrixXf& Blist, const Eigen::MatrixXf& thetaList) {
		Eigen::MatrixXf Jb = Blist;
		Eigen::MatrixXf T = Eigen::MatrixXf::Identity(4, 4);
		Eigen::VectorXf bListTemp(Blist.col(0).size());
		for (int i = thetaList.size() - 2; i >= 0; i--) {
			bListTemp << Blist.col(i + 1) * thetaList(i + 1);
			T = T * MatrixExp6(VecTose3(-1 * bListTemp));
			// std::cout << "array: " << sListTemp << std::endl;
			Jb.col(i) = Adjoint(T) * Blist.col(i);
		}
		return Jb;
	}

	Eigen::MatrixXf TransInv(const Eigen::MatrixXf& transform) {
		auto rp = mr::TransToRp(transform);
		auto Rt = rp.at(0).transpose();
		auto t = -(Rt * rp.at(1));
		Eigen::MatrixXf inv(4, 4);
		inv = Eigen::MatrixXf::Zero(4,4);
		inv.block(0, 0, 3, 3) = Rt;
		inv.block(0, 3, 3, 1) = t;
		inv(3, 3) = 1;
		return inv;
	}

	Eigen::MatrixXf RotInv(const Eigen::MatrixXf& rotMatrix) {
		return rotMatrix.transpose();
	}

	Eigen::VectorXf ScrewToAxis(Eigen::Vector3f q, Eigen::Vector3f s, float h) {
		Eigen::VectorXf axis(6);
		axis.segment(0, 3) = s;
		axis.segment(3, 3) = q.cross(s) + (h * s);
		return axis;
	}

	Eigen::VectorXf AxisAng6(const Eigen::VectorXf& expc6) {
		Eigen::VectorXf v_ret(7);
		float theta = Eigen::Vector3f(expc6(0), expc6(1), expc6(2)).norm();
		if (NearZero(theta))
			theta = Eigen::Vector3f(expc6(3), expc6(4), expc6(5)).norm();
		v_ret << expc6 / theta, theta;
		return v_ret;
	}

	Eigen::MatrixXf ProjectToSO3(const Eigen::MatrixXf& M) {
		Eigen::JacobiSVD<Eigen::MatrixXf> svd(M, Eigen::ComputeFullU | Eigen::ComputeFullV);
		Eigen::MatrixXf R = svd.matrixU() * svd.matrixV().transpose();
		if (R.determinant() < 0)
			// In this case the result may be far from M; reverse sign of 3rd column
			R.col(2) *= -1;
		return R;
	}

	Eigen::MatrixXf ProjectToSE3(const Eigen::MatrixXf& M) {
		Eigen::Matrix3f R = M.block<3, 3>(0, 0);
		Eigen::Vector3f t = M.block<3, 1>(0, 3);
		Eigen::MatrixXf T = RpToTrans(ProjectToSO3(R), t);
		return T;
	}

	float DistanceToSO3(const Eigen::Matrix3f& M) {
		if (M.determinant() > 0)
			return (M.transpose() * M - Eigen::Matrix3f::Identity()).norm();
		else
			return 1.0e9;
	}

	float DistanceToSE3(const Eigen::Matrix4f& T) {
		Eigen::Matrix3f matR = T.block<3, 3>(0, 0);
		if (matR.determinant() > 0) {
			Eigen::Matrix4f m_ret;
			m_ret << matR.transpose()*matR, Eigen::Vector3f::Zero(3),
				T.row(3);
			m_ret = m_ret - Eigen::Matrix4f::Identity();
			return m_ret.norm();
		}
		else
			return 1.0e9;
	}

	bool TestIfSO3(const Eigen::Matrix3f& M) {
		return std::abs(DistanceToSO3(M)) < 1e-3;
	}

	bool TestIfSE3(const Eigen::Matrix4f& T) {
		return std::abs(DistanceToSE3(T)) < 1e-3;
	}
	bool IKinBody(const Eigen::MatrixXf& Blist, const Eigen::MatrixXf& M, const Eigen::MatrixXf& T,
		Eigen::VectorXf& thetalist, float eomg, float ev) {
		int i = 0;
		int maxiterations = 20;
		Eigen::MatrixXf Tfk = FKinBody(M, Blist, thetalist);
		Eigen::MatrixXf Tdiff = TransInv(Tfk)*T;
		Eigen::VectorXf Vb = se3ToVec(MatrixLog6(Tdiff));
		Eigen::Vector3f angular(Vb(0), Vb(1), Vb(2));
		Eigen::Vector3f linear(Vb(3), Vb(4), Vb(5));

		bool err = (angular.norm() > eomg || linear.norm() > ev);
		Eigen::MatrixXf Jb;
		while (err && i < maxiterations) {
			Jb = JacobianBody(Blist, thetalist);
			thetalist += Jb.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(Vb);
			i += 1;
			// iterate
			Tfk = FKinBody(M, Blist, thetalist);
			Tdiff = TransInv(Tfk)*T;
			Vb = se3ToVec(MatrixLog6(Tdiff));
			angular = Eigen::Vector3f(Vb(0), Vb(1), Vb(2));
			linear = Eigen::Vector3f(Vb(3), Vb(4), Vb(5));
			err = (angular.norm() > eomg || linear.norm() > ev);
		}
		return !err;
	}

	bool IKinSpace(const Eigen::MatrixXf& Slist, const Eigen::MatrixXf& M, const Eigen::MatrixXf& T,
		Eigen::VectorXf& thetalist, float eomg, float ev) {
		int i = 0;
		int maxiterations = 20;
		Eigen::MatrixXf Tfk = FKinSpace(M, Slist, thetalist);
		Eigen::MatrixXf Tdiff = TransInv(Tfk)*T;
		Eigen::VectorXf Vs = Adjoint(Tfk)*se3ToVec(MatrixLog6(Tdiff));
		Eigen::Vector3f angular(Vs(0), Vs(1), Vs(2));
		Eigen::Vector3f linear(Vs(3), Vs(4), Vs(5));

		bool err = (angular.norm() > eomg || linear.norm() > ev);
		Eigen::MatrixXf Js;
		while (err && i < maxiterations) {
			Js = JacobianSpace(Slist, thetalist);
			thetalist += Js.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(Vs);
			i += 1;
			// iterate
			Tfk = FKinSpace(M, Slist, thetalist);
			Tdiff = TransInv(Tfk)*T;
			Vs = Adjoint(Tfk)*se3ToVec(MatrixLog6(Tdiff));
			angular = Eigen::Vector3f(Vs(0), Vs(1), Vs(2));
			linear = Eigen::Vector3f(Vs(3), Vs(4), Vs(5));
			err = (angular.norm() > eomg || linear.norm() > ev);
		}
		return !err;
	}

	/*
	* Function: This function uses forward-backward Newton-Euler iterations to solve the
	* equation:
	* taulist = Mlist(thetalist) * ddthetalist + c(thetalist, dthetalist) ...
	*           + g(thetalist) + Jtr(thetalist) * Ftip
	* Inputs:
	*  thetalist: n-vector of joint variables
	*  dthetalist: n-vector of joint rates
	*  ddthetalist: n-vector of joint accelerations
	*  g: Gravity vector g
	*  Ftip: Spatial force applied by the end-effector expressed in frame {n+1}
	*  Mlist: List of link frames {i} relative to {i-1} at the home position
	*  Glist: Spatial inertia matrices Gi of the links
	*  Slist: Screw axes Si of the joints in a space frame, in the format
	*         of a matrix with the screw axes as the columns.
	*
	* Outputs:
	*  taulist: The n-vector of required joint forces/torques
	*
	*/
	Eigen::VectorXf InverseDynamics(const Eigen::VectorXf& thetalist, const Eigen::VectorXf& dthetalist, const Eigen::VectorXf& ddthetalist,
									const Eigen::VectorXf& g, const Eigen::VectorXf& Ftip, const std::vector<Eigen::MatrixXf>& Mlist,
									const std::vector<Eigen::MatrixXf>& Glist, const Eigen::MatrixXf& Slist) {
	    // the size of the lists
		int n = thetalist.size();

		Eigen::MatrixXf Mi = Eigen::MatrixXf::Identity(4, 4);
		Eigen::MatrixXf Ai = Eigen::MatrixXf::Zero(6,n);
		std::vector<Eigen::MatrixXf> AdTi;
		for (int i = 0; i < n+1; i++) {
			AdTi.push_back(Eigen::MatrixXf::Zero(6,6));
		}
		Eigen::MatrixXf Vi = Eigen::MatrixXf::Zero(6,n+1);    // velocity
		Eigen::MatrixXf Vdi = Eigen::MatrixXf::Zero(6,n+1);   // acceleration

		Vdi.block(3, 0, 3, 1) = - g;
		AdTi[n] = mr::Adjoint(mr::TransInv(Mlist[n]));
		Eigen::VectorXf Fi = Ftip;

		Eigen::VectorXf taulist = Eigen::VectorXf::Zero(n);

		// forward pass
		for (int i = 0; i < n; i++) {
			Mi = Mi * Mlist[i];
			Ai.col(i) = mr::Adjoint(mr::TransInv(Mi))*Slist.col(i);

			AdTi[i] = mr::Adjoint(mr::MatrixExp6(mr::VecTose3(Ai.col(i)*-thetalist(i)))
			          * mr::TransInv(Mlist[i]));

			Vi.col(i+1) = AdTi[i] * Vi.col(i) + Ai.col(i) * dthetalist(i);
			Vdi.col(i+1) = AdTi[i] * Vdi.col(i) + Ai.col(i) * ddthetalist(i)
						   + ad(Vi.col(i+1)) * Ai.col(i) * dthetalist(i); // this index is different from book!
		}

		// backward pass
		for (int i = n-1; i >= 0; i--) {
			Fi = AdTi[i+1].transpose() * Fi + Glist[i] * Vdi.col(i+1)
			     - ad(Vi.col(i+1)).transpose() * (Glist[i] * Vi.col(i+1));
			taulist(i) = Fi.transpose() * Ai.col(i);
		}
		return taulist;
	}

	/*
	 * Function: This function calls InverseDynamics with Ftip = 0, dthetalist = 0, and
	 *   ddthetalist = 0. The purpose is to calculate one important term in the dynamics equation
	 * Inputs:
	 *  thetalist: n-vector of joint variables
	 *  g: Gravity vector g
	 *  Mlist: List of link frames {i} relative to {i-1} at the home position
	 *  Glist: Spatial inertia matrices Gi of the links
	 *  Slist: Screw axes Si of the joints in a space frame, in the format
	 *         of a matrix with the screw axes as the columns.
	 *
	 * Outputs:
	 *  grav: The 3-vector showing the effect force of gravity to the dynamics
	 *
	 */
	Eigen::VectorXf GravityForces(const Eigen::VectorXf& thetalist, const Eigen::VectorXf& g,
									const std::vector<Eigen::MatrixXf>& Mlist, const std::vector<Eigen::MatrixXf>& Glist, const Eigen::MatrixXf& Slist) {
	    int n = thetalist.size();
		Eigen::VectorXf dummylist = Eigen::VectorXf::Zero(n);
		Eigen::VectorXf dummyForce = Eigen::VectorXf::Zero(6);
		Eigen::VectorXf grav = mr::InverseDynamics(thetalist, dummylist, dummylist, g,
                                                dummyForce, Mlist, Glist, Slist);
		return grav;
	}

	/*
  	 * Function: This function calls InverseDynamics n times, each time passing a
	 * ddthetalist vector with a single element equal to one and all other
	 * inputs set to zero. Each call of InverseDynamics generates a single
	 * column, and these columns are assembled to create the inertia matrix.
	 *
	 * Inputs:
	 *  thetalist: n-vector of joint variables
	 *  Mlist: List of link frames {i} relative to {i-1} at the home position
	 *  Glist: Spatial inertia matrices Gi of the links
	 *  Slist: Screw axes Si of the joints in a space frame, in the format
	 *         of a matrix with the screw axes as the columns.
	 *
	 * Outputs:
	 *  M: The numerical inertia matrix M(thetalist) of an n-joint serial
	 *     chain at the given configuration thetalist.
	 */
	Eigen::MatrixXf MassMatrix(const Eigen::VectorXf& thetalist,
                                const std::vector<Eigen::MatrixXf>& Mlist, const std::vector<Eigen::MatrixXf>& Glist, const Eigen::MatrixXf& Slist) {
		int n = thetalist.size();
		Eigen::VectorXf dummylist = Eigen::VectorXf::Zero(n);
		Eigen::VectorXf dummyg = Eigen::VectorXf::Zero(3);
		Eigen::VectorXf dummyforce = Eigen::VectorXf::Zero(6);
		Eigen::MatrixXf M = Eigen::MatrixXf::Zero(n,n);
		for (int i = 0; i < n; i++) {
			Eigen::VectorXf ddthetalist = Eigen::VectorXf::Zero(n);
			ddthetalist(i) = 1;
			M.col(i) = mr::InverseDynamics(thetalist, dummylist, ddthetalist,
                             dummyg, dummyforce, Mlist, Glist, Slist);
		}
		return M;
	}

	/*
  	 * Function: This function calls InverseDynamics with g = 0, Ftip = 0, and
     * ddthetalist = 0.
	 *
	 * Inputs:
	 *  thetalist: n-vector of joint variables
	 *  dthetalist: A list of joint rates
	 *  Mlist: List of link frames {i} relative to {i-1} at the home position
	 *  Glist: Spatial inertia matrices Gi of the links
	 *  Slist: Screw axes Si of the joints in a space frame, in the format
	 *         of a matrix with the screw axes as the columns.
	 *
	 * Outputs:
	 *  c: The vector c(thetalist,dthetalist) of Coriolis and centripetal
	 *     terms for a given thetalist and dthetalist.
	 */
	Eigen::VectorXf VelQuadraticForces(const Eigen::VectorXf& thetalist, const Eigen::VectorXf& dthetalist,
                                const std::vector<Eigen::MatrixXf>& Mlist, const std::vector<Eigen::MatrixXf>& Glist, const Eigen::MatrixXf& Slist) {
		int n = thetalist.size();
		Eigen::VectorXf dummylist = Eigen::VectorXf::Zero(n);
		Eigen::VectorXf dummyg = Eigen::VectorXf::Zero(3);
		Eigen::VectorXf dummyforce = Eigen::VectorXf::Zero(6);
		Eigen::VectorXf c = mr::InverseDynamics(thetalist, dthetalist, dummylist,
                             dummyg, dummyforce, Mlist, Glist, Slist);
		return c;
	}

	/*
  	 * Function: This function calls InverseDynamics with g = 0, dthetalist = 0, and
     * ddthetalist = 0.
	 *
	 * Inputs:
	 *  thetalist: n-vector of joint variables
	 *  Ftip: Spatial force applied by the end-effector expressed in frame {n+1}
	 *  Mlist: List of link frames {i} relative to {i-1} at the home position
	 *  Glist: Spatial inertia matrices Gi of the links
	 *  Slist: Screw axes Si of the joints in a space frame, in the format
	 *         of a matrix with the screw axes as the columns.
	 *
	 * Outputs:
	 *  JTFtip: The joint forces and torques required only to create the
	 *     end-effector force Ftip.
	 */
	Eigen::VectorXf EndEffectorForces(const Eigen::VectorXf& thetalist, const Eigen::VectorXf& Ftip,
								const std::vector<Eigen::MatrixXf>& Mlist, const std::vector<Eigen::MatrixXf>& Glist, const Eigen::MatrixXf& Slist) {
		int n = thetalist.size();
		Eigen::VectorXf dummylist = Eigen::VectorXf::Zero(n);
		Eigen::VectorXf dummyg = Eigen::VectorXf::Zero(3);

		Eigen::VectorXf JTFtip = mr::InverseDynamics(thetalist, dummylist, dummylist,
                             dummyg, Ftip, Mlist, Glist, Slist);
		return JTFtip;
	}

	/*
	 * Function: This function computes ddthetalist by solving:
	 * Mlist(thetalist) * ddthetalist = taulist - c(thetalist,dthetalist)
	 *                                  - g(thetalist) - Jtr(thetalist) * Ftip
	 * Inputs:
	 *  thetalist: n-vector of joint variables
	 *  dthetalist: n-vector of joint rates
	 *  taulist: An n-vector of joint forces/torques
	 *  g: Gravity vector g
	 *  Ftip: Spatial force applied by the end-effector expressed in frame {n+1}
	 *  Mlist: List of link frames {i} relative to {i-1} at the home position
	 *  Glist: Spatial inertia matrices Gi of the links
	 *  Slist: Screw axes Si of the joints in a space frame, in the format
	 *         of a matrix with the screw axes as the columns.
	 *
	 * Outputs:
	 *  ddthetalist: The resulting joint accelerations
	 *
	 */
	Eigen::VectorXf ForwardDynamics(const Eigen::VectorXf& thetalist, const Eigen::VectorXf& dthetalist, const Eigen::VectorXf& taulist,
									const Eigen::VectorXf& g, const Eigen::VectorXf& Ftip, const std::vector<Eigen::MatrixXf>& Mlist,
									const std::vector<Eigen::MatrixXf>& Glist, const Eigen::MatrixXf& Slist) {

		Eigen::VectorXf totalForce = taulist - mr::VelQuadraticForces(thetalist, dthetalist, Mlist, Glist, Slist)
                 							 - mr::GravityForces(thetalist, g, Mlist, Glist, Slist)
                                             - mr::EndEffectorForces(thetalist, Ftip, Mlist, Glist, Slist);

		Eigen::MatrixXf M = mr::MassMatrix(thetalist, Mlist, Glist, Slist);

		// Use LDLT since M is positive definite
        Eigen::VectorXf ddthetalist = M.ldlt().solve(totalForce);

		return ddthetalist;
	}

	void EulerStep(Eigen::VectorXf& thetalist, Eigen::VectorXf& dthetalist, const Eigen::VectorXf& ddthetalist, float dt) {
		thetalist += dthetalist * dt;
		dthetalist += ddthetalist * dt;
		return;
	}

	Eigen::MatrixXf InverseDynamicsTrajectory(const Eigen::MatrixXf& thetamat, const Eigen::MatrixXf& dthetamat, const Eigen::MatrixXf& ddthetamat,
		const Eigen::VectorXf& g, const Eigen::MatrixXf& Ftipmat, const std::vector<Eigen::MatrixXf>& Mlist, const std::vector<Eigen::MatrixXf>& Glist,
		const Eigen::MatrixXf& Slist) {
		Eigen::MatrixXf thetamatT = thetamat.transpose();
		Eigen::MatrixXf dthetamatT = dthetamat.transpose();
		Eigen::MatrixXf ddthetamatT = ddthetamat.transpose();
		Eigen::MatrixXf FtipmatT = Ftipmat.transpose();

		int N = thetamat.rows();  // trajectory points
		int dof = thetamat.cols();
		Eigen::MatrixXf taumatT = Eigen::MatrixXf::Zero(dof, N);
		for (int i = 0; i < N; ++i) {
			taumatT.col(i) = InverseDynamics(thetamatT.col(i), dthetamatT.col(i), ddthetamatT.col(i), g, FtipmatT.col(i), Mlist, Glist, Slist);
		}
		Eigen::MatrixXf taumat = taumatT.transpose();
		return taumat;
	}

	std::vector<Eigen::MatrixXf> ForwardDynamicsTrajectory(const Eigen::VectorXf& thetalist, const Eigen::VectorXf& dthetalist, const Eigen::MatrixXf& taumat,
		const Eigen::VectorXf& g, const Eigen::MatrixXf& Ftipmat, const std::vector<Eigen::MatrixXf>& Mlist, const std::vector<Eigen::MatrixXf>& Glist,
		const Eigen::MatrixXf& Slist, float dt, int intRes) {
		Eigen::MatrixXf taumatT = taumat.transpose();
		Eigen::MatrixXf FtipmatT = Ftipmat.transpose();
		int N = taumat.rows();  // force/torque points
		int dof = taumat.cols();
		Eigen::MatrixXf thetamatT = Eigen::MatrixXf::Zero(dof, N);
		Eigen::MatrixXf dthetamatT = Eigen::MatrixXf::Zero(dof, N);
		thetamatT.col(0) = thetalist;
		dthetamatT.col(0) = dthetalist;
		Eigen::VectorXf thetacurrent = thetalist;
		Eigen::VectorXf dthetacurrent = dthetalist;
		Eigen::VectorXf ddthetalist;
		for (int i = 0; i < N - 1; ++i) {
			for (int j = 0; j < intRes; ++j) {
				ddthetalist = ForwardDynamics(thetacurrent, dthetacurrent, taumatT.col(i), g, FtipmatT.col(i), Mlist, Glist, Slist);
				EulerStep(thetacurrent, dthetacurrent, ddthetalist, 1.0*dt / intRes);
			}
			thetamatT.col(i + 1) = thetacurrent;
			dthetamatT.col(i + 1) = dthetacurrent;
		}
		std::vector<Eigen::MatrixXf> JointTraj_ret;
		JointTraj_ret.push_back(thetamatT.transpose());
		JointTraj_ret.push_back(dthetamatT.transpose());
		return JointTraj_ret;
	}

	Eigen::VectorXf ComputedTorque(const Eigen::VectorXf& thetalist, const Eigen::VectorXf& dthetalist, const Eigen::VectorXf& eint,
		const Eigen::VectorXf& g, const std::vector<Eigen::MatrixXf>& Mlist, const std::vector<Eigen::MatrixXf>& Glist,
		const Eigen::MatrixXf& Slist, const Eigen::VectorXf& thetalistd, const Eigen::VectorXf& dthetalistd, const Eigen::VectorXf& ddthetalistd,
		float Kp, float Ki, float Kd) {

		Eigen::VectorXf e = thetalistd - thetalist;  // position err
		Eigen::VectorXf tau_feedforward = MassMatrix(thetalist, Mlist, Glist, Slist)*(Kp*e + Ki * (eint + e) + Kd * (dthetalistd - dthetalist));

		Eigen::VectorXf Ftip = Eigen::VectorXf::Zero(6);
		Eigen::VectorXf tau_inversedyn = InverseDynamics(thetalist, dthetalist, ddthetalistd, g, Ftip, Mlist, Glist, Slist);

		Eigen::VectorXf tau_computed = tau_feedforward + tau_inversedyn;
		return tau_computed;
	}

	float CubicTimeScaling(float Tf, float t) {
		float timeratio = 1.0*t / Tf;
		float st = 3 * pow(timeratio, 2) - 2 * pow(timeratio, 3);
		return st;
	}

	float QuinticTimeScaling(float Tf, float t) {
		float timeratio = 1.0*t / Tf;
		float st = 10 * pow(timeratio, 3) - 15 * pow(timeratio, 4) + 6 * pow(timeratio, 5);
		return st;
	}

	Eigen::MatrixXf JointTrajectory(const Eigen::VectorXf& thetastart, const Eigen::VectorXf& thetaend, float Tf, int N, int method) {
		float timegap = Tf / (N - 1);
		Eigen::MatrixXf trajT = Eigen::MatrixXf::Zero(thetastart.size(), N);
		float st;
		for (int i = 0; i < N; ++i) {
			if (method == 3)
				st = CubicTimeScaling(Tf, timegap*i);
			else
				st = QuinticTimeScaling(Tf, timegap*i);
			trajT.col(i) = st * thetaend + (1 - st)*thetastart;
		}
		Eigen::MatrixXf traj = trajT.transpose();
		return traj;
	}
	std::vector<Eigen::MatrixXf> ScrewTrajectory(const Eigen::MatrixXf& Xstart, const Eigen::MatrixXf& Xend, float Tf, int N, int method) {
		float timegap = Tf / (N - 1);
		std::vector<Eigen::MatrixXf> traj(N);
		float st;
		for (int i = 0; i < N; ++i) {
			if (method == 3)
				st = CubicTimeScaling(Tf, timegap*i);
			else
				st = QuinticTimeScaling(Tf, timegap*i);
			Eigen::MatrixXf Ttemp = MatrixLog6(TransInv(Xstart)*Xend);
			traj.at(i) = Xstart * MatrixExp6(Ttemp*st);
		}
		return traj;
	}

	std::vector<Eigen::MatrixXf> CartesianTrajectory(const Eigen::MatrixXf& Xstart, const Eigen::MatrixXf& Xend, float Tf, int N, int method) {
		float timegap = Tf / (N - 1);
		std::vector<Eigen::MatrixXf> traj(N);
		std::vector<Eigen::MatrixXf> Rpstart = TransToRp(Xstart);
		std::vector<Eigen::MatrixXf> Rpend = TransToRp(Xend);
		Eigen::Matrix3f Rstart = Rpstart[0]; Eigen::Vector3f pstart = Rpstart[1];
		Eigen::Matrix3f Rend = Rpend[0]; Eigen::Vector3f pend = Rpend[1];
		float st;
		for (int i = 0; i < N; ++i) {
			if (method == 3)
				st = CubicTimeScaling(Tf, timegap*i);
			else
				st = QuinticTimeScaling(Tf, timegap*i);
			Eigen::Matrix3f Ri = Rstart * MatrixExp3(MatrixLog3(Rstart.transpose() * Rend)*st);
			Eigen::Vector3f pi = st*pend + (1 - st)*pstart;
			Eigen::MatrixXf traji(4, 4);
			traji << Ri, pi,
				0, 0, 0, 1;
			traj.at(i) = traji;
		}
		return traj;
	}
	std::vector<Eigen::MatrixXf> SimulateControl(const Eigen::VectorXf& thetalist, const Eigen::VectorXf& dthetalist, const Eigen::VectorXf& g,
		const Eigen::MatrixXf& Ftipmat, const std::vector<Eigen::MatrixXf>& Mlist, const std::vector<Eigen::MatrixXf>& Glist,
		const Eigen::MatrixXf& Slist, const Eigen::MatrixXf& thetamatd, const Eigen::MatrixXf& dthetamatd, const Eigen::MatrixXf& ddthetamatd,
		const Eigen::VectorXf& gtilde, const std::vector<Eigen::MatrixXf>& Mtildelist, const std::vector<Eigen::MatrixXf>& Gtildelist,
		float Kp, float Ki, float Kd, float dt, int intRes) {
		Eigen::MatrixXf FtipmatT = Ftipmat.transpose();
		Eigen::MatrixXf thetamatdT = thetamatd.transpose();
		Eigen::MatrixXf dthetamatdT = dthetamatd.transpose();
		Eigen::MatrixXf ddthetamatdT = ddthetamatd.transpose();
		int m = thetamatdT.rows(); int n = thetamatdT.cols();
		Eigen::VectorXf thetacurrent = thetalist;
		Eigen::VectorXf dthetacurrent = dthetalist;
		Eigen::VectorXf eint = Eigen::VectorXf::Zero(m);
		Eigen::MatrixXf taumatT = Eigen::MatrixXf::Zero(m, n);
		Eigen::MatrixXf thetamatT = Eigen::MatrixXf::Zero(m, n);
		Eigen::VectorXf taulist;
		Eigen::VectorXf ddthetalist;
		for (int i = 0; i < n; ++i) {
			taulist = ComputedTorque(thetacurrent, dthetacurrent, eint, gtilde, Mtildelist, Gtildelist, Slist, thetamatdT.col(i),
				dthetamatdT.col(i), ddthetamatdT.col(i), Kp, Ki, Kd);
			for (int j = 0; j < intRes; ++j) {
				ddthetalist = ForwardDynamics(thetacurrent, dthetacurrent, taulist, g, FtipmatT.col(i), Mlist, Glist, Slist);
				EulerStep(thetacurrent, dthetacurrent, ddthetalist, dt / intRes);
			}
			taumatT.col(i) = taulist;
			thetamatT.col(i) = thetacurrent;
			eint += dt * (thetamatdT.col(i) - thetacurrent);
		}
		std::vector<Eigen::MatrixXf> ControlTauTraj_ret;
		ControlTauTraj_ret.push_back(taumatT.transpose());
		ControlTauTraj_ret.push_back(thetamatT.transpose());
		return ControlTauTraj_ret;
	}
}
