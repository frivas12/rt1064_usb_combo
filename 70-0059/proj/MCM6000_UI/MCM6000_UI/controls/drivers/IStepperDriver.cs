using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace MCM6000_UI.controls.drivers
{
    public interface IStepperDriver : IDriverBase
    {
        public interface ISavedPosition
        {
            public int Index
            { get; }

            public int Position
            { get; set; }

            public void ClearPosition();
        }

        public enum Direction
        {
            CCW = 0,
            CW = 1,

            Backwards = 0,
            Forwards = 1,
        }

        public int EncoderPosition
        { get; }

        public uint RawEncoderPosition
        { get; }

        public int StepperPosition
        { get; }

        public double NmPerEncoderCount
        { get; }

        public double StepperCountsPerEncoderCount
        { get; }

        public bool MovingCCW { get; }
        public bool MovingCW { get; }
        public bool HardLimitCCW { get; }
        public bool HardLimitCW { get; }
        public bool SoftLimitCCW { get; }
        public bool SoftLimitCW { get; }

        public bool NotHomed { get; }
        public bool Homing { get; }
        public bool Homed { get; }

        public bool IsDeviceConnected { get; }

        public IEnumerable<ISavedPosition> SavedPositions { get; }
        public ISavedPosition CurrentSavedPosition { get; }

        /**
         * Sent to begin a homing move.
         */
        public void DispatchHome();

        /**
         * Sent to zero the encoder count (for relative encoders).
         */
        public Task ZeroEncoderAsync(CancellationToken token);

        /**
         * Sent to stop the motor.
         */
        public Task StopAsync(CancellationToken token);

        /**
         * Sent to set the current position as the soft limit for the given direction.
         */
        public Task SetSoftLimitAsync(Direction dir, CancellationToken token);

        /**
         * Sent to clear the soft limits.
         */
        public Task ClearSoftLimitsAsync(CancellationToken token);

        /**
         * Sent to move the stage to an encoder position.
         */
        public void DispatchGoto(int position);

        /**
         * Sent to jog in the specified direction.
         */
        public void DispatchJog(Direction dir);

        /**
         * Sent to set the jog step size.
         */
        public Task SaveJogStepSizeAsync(int step_size, CancellationToken token);

        /**
         * Sent to move to a set position.
         */
        public void DispatchMoveToPosition(ISavedPosition position);

        /**
         * Sent to move by a certain amount in encoder counts.
         */
        public void DispatchMoveBy(int delta);

        /**
         * Sent to begin a velocity move.
         */
        public void DispatchMoveDir(Direction dir, byte velocity_percent);

        /**
         * Commits the saved positions to the remote device.
         */
        public Task CommitSavedPositionsAsync(CancellationToken token);

        /**
         * Overwrites the saved positions with the device's value.
         */
        public Task RestoreSavedPositionsAsync(CancellationToken token);
    }
}
