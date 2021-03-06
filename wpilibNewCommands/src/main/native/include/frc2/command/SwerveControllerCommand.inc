/*----------------------------------------------------------------------------*/
/* Copyright (c) 2019-2020 FIRST. All Rights Reserved.                        */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#pragma once

#include <memory>

namespace frc2 {

template <size_t NumModules>
SwerveControllerCommand<NumModules>::SwerveControllerCommand(
    frc::Trajectory trajectory, std::function<frc::Pose2d()> pose,
    frc::SwerveDriveKinematics<NumModules> kinematics,
    frc2::PIDController xController, frc2::PIDController yController,
    frc::ProfiledPIDController<units::radians> thetaController,
    std::function<void(std::array<frc::SwerveModuleState, NumModules>)> output,
    std::initializer_list<Subsystem*> requirements)
    : m_trajectory(trajectory),
      m_pose(pose),
      m_kinematics(kinematics),
      m_xController(std::make_unique<frc2::PIDController>(xController)),
      m_yController(std::make_unique<frc2::PIDController>(yController)),
      m_thetaController(
          std::make_unique<frc::ProfiledPIDController<units::radians>>(
              thetaController)),
      m_outputStates(output) {
  this->AddRequirements(requirements);
}

template <size_t NumModules>
SwerveControllerCommand<NumModules>::SwerveControllerCommand(
    frc::Trajectory trajectory, std::function<frc::Pose2d()> pose,
    frc::SwerveDriveKinematics<NumModules> kinematics,
    frc2::PIDController xController, frc2::PIDController yController,
    frc::ProfiledPIDController<units::radians> thetaController,
    std::function<void(std::array<frc::SwerveModuleState, NumModules>)> output,
    wpi::ArrayRef<Subsystem*> requirements)
    : m_trajectory(trajectory),
      m_pose(pose),
      m_kinematics(kinematics),
      m_xController(std::make_unique<frc2::PIDController>(xController)),
      m_yController(std::make_unique<frc2::PIDController>(yController)),
      m_thetaController(
          std::make_unique<frc::ProfiledPIDController<units::radians>>(
              thetaController)),
      m_outputStates(output) {
  this->AddRequirements(requirements);
}

template <size_t NumModules>
void SwerveControllerCommand<NumModules>::Initialize() {
  m_finalPose = m_trajectory.Sample(m_trajectory.TotalTime()).pose;

  m_timer.Reset();
  m_timer.Start();
}

template <size_t NumModules>
void SwerveControllerCommand<NumModules>::Execute() {
  auto curTime = units::second_t(m_timer.Get());

  auto m_desiredState = m_trajectory.Sample(curTime);
  auto m_desiredPose = m_desiredState.pose;

  auto m_poseError = m_desiredPose.RelativeTo(m_pose());

  auto targetXVel = units::meters_per_second_t(m_xController->Calculate(
      (m_pose().Translation().X().template to<double>()),
      (m_desiredPose.Translation().X().template to<double>())));
  auto targetYVel = units::meters_per_second_t(m_yController->Calculate(
      (m_pose().Translation().Y().template to<double>()),
      (m_desiredPose.Translation().Y().template to<double>())));

  // Profiled PID Controller only takes meters as setpoint and measurement
  // The robot will go to the desired rotation of the final pose in the
  // trajectory, not following the poses at individual states.
  auto targetAngularVel =
      units::radians_per_second_t(m_thetaController->Calculate(
          m_pose().Rotation().Radians(), m_finalPose.Rotation().Radians()));

  auto vRef = m_desiredState.velocity;

  targetXVel += vRef * m_poseError.Rotation().Cos();
  targetYVel += vRef * m_poseError.Rotation().Sin();

  auto targetChassisSpeeds =
      frc::ChassisSpeeds{targetXVel, targetYVel, targetAngularVel};

  auto targetModuleStates =
      m_kinematics.ToSwerveModuleStates(targetChassisSpeeds);

  m_outputStates(targetModuleStates);
}

template <size_t NumModules>
void SwerveControllerCommand<NumModules>::End(bool interrupted) {
  m_timer.Stop();
}

template <size_t NumModules>
bool SwerveControllerCommand<NumModules>::IsFinished() {
  return m_timer.HasElapsed(m_trajectory.TotalTime());
}

}  // namespace frc2
