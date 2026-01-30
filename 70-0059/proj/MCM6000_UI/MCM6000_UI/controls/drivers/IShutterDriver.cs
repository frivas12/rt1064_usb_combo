using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;

namespace MCM6000_UI.controls.drivers
{
    public interface IShutterDriver : IDriverBase
    {
        public enum States
        {
            OPEN = 0,
            CLOSED = 1,
            UNKNOWN = 2,
        }
        public enum Types
        {
            DISABLED = 0,
            PULSED = 1,
            BIDIRECTIONAL_PULSE_HOLD = 2,
            UNIDIRECTIONAL_PULSE_HOLD = 3,
        }
        public enum TriggerModes
        {
            DISABLED = 0,
            ENABLED = 1,
            INVERTED = 2,
        }

        public bool IsEnabled
        { get; }

        public States State
        { get; }

        public TriggerModes ExternalTriggerMode
        { get; }


        public Types Type
        { get; }

        public States StateAtPowerUp
        { get; }

        public double PulseDutyCyclePercentage
        { get; }

        public double HoldDutyCyclePercentage
        { get; }

        public TimeSpan PulseWidth
        { get; }


        /// <summary>
        /// Converts the percentage duty cycle into volts.
        /// </summary>
        /// <exception cref="ArgumentException">The percentage was invalid</exception>
        public double DutyCycleToVoltage(double percentage);

        /// <summary>
        /// Converts the voltage into the percentage duty cycle.
        /// </summary>
        /// <exception cref="ArgumentException">The voltage was out of range.</exception>
        public double VoltageToDutyCycle(double voltage);


        public Task SetEnableAsync(bool enable, CancellationToken token);
        public Task SetStateAsync(States state, CancellationToken token);
        public Task SetExternalTriggerModeAsync(TriggerModes mode, CancellationToken token);

        /// <summary>
        /// Configures the driver to not drive.
        /// </summary>
        public Task ConfigureNoDriver(CancellationToken token);

        /// <summary>
        /// Configures the driver as a pulsed shutter.
        /// </summary>
        public Task ConfigurePulsed(
            double pulseDutyCyclePercentage, TimeSpan pulseWidth,
            States stateAtPowerUp, CancellationToken token);

        /// <summary>
        /// Configures the driver as a bidirectional pulse hold shutter.
        /// </summary>
        public Task ConfigureBidirectionalPulseHold(
            double pulseDutyCyclePercentage, TimeSpan pulseWidth,
            double holdDutyCyclePercentage,
            States stateAtPowerUp, CancellationToken token);

        /// <summary>
        /// Configures the ddriver as a unidirection pulse hold shutter.
        /// </summary>
        public Task ConfigureUnidirectionalPulseHold(
            double pulseDutyCyclePercentage, TimeSpan pulseWidth,
            double holdDutyCyclePercentage,
            States stateAtPowerUp, CancellationToken token);
    }
}
