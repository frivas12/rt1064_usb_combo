using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reactive.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;

namespace MCM6000_UI.controls.vms
{
    public class ShutterConfiguratorVM : ReactiveObject
    {
        public int Channel => _channel.Value;
        public string ChannelName => _channelName.Value;

        public drivers.IShutterDriver.Types Type => _type.Value;
        public drivers.IShutterDriver.States StateAtPowerUp => _stateAtPowerUp.Value;
        public double PulseVoltage => _pulseVoltage.Value;
        public double HoldVoltage => _holdVoltage.Value;
        public TimeSpan PulseWidth => _pulseWidth.Value;

        public drivers.IShutterDriver.Types TypeScratchpad
        {
            get => _typeScratchpad;
            set => this.RaiseAndSetIfChanged(ref _typeScratchpad, value);
        }
        public drivers.IShutterDriver.States StateAtPowerUpScratchpad
        {
            get => _stateAtPowerUpScratchpad;
            set => this.RaiseAndSetIfChanged(ref _stateAtPowerUpScratchpad, value);
        }
        public double PulseVoltageScratchpad
        {
            get => _pulseVoltageScratchpad;
            set => this.RaiseAndSetIfChanged(ref _pulseVoltageScratchpad, value);
        }
        public bool PulseVoltageScratchpadEnabled => _pulseVoltageScratchpadEnabled.Value;
        public double HoldVoltageScratchpad
        {
            get => _holdVoltageScratchpad;
            set => this.RaiseAndSetIfChanged(ref _holdVoltageScratchpad, value);
        }
        public bool HoldVoltageScratchpadEnabled => _holdVoltageScratchpadEnabled.Value;
        public TimeSpan PulseWidthScratchpad
        {
            get => _pulseWidthScratchpad;
            set => this.RaiseAndSetIfChanged(ref _pulseWidthScratchpad, value);
        }
        public bool PulseWidthScratchpadEnabled => _pulseWidthScratchpadEnabled.Value;

        public ICommand CommitSettings
        { get; }

        public ICommand RevertSettings
        { get; }


        public drivers.IShutterDriver Driver
        {
            get => _driver;
            set => this.RaiseAndSetIfChanged(ref _driver, value);
        }


        public ShutterConfiguratorVM()
        {
            _channel = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(-1) :
                    this.WhenAnyValue(x => x.Driver.Channel))
                .Switch()
                .ToProperty(this, x => x.Channel);
            _channelName = this
                .WhenAnyValue(x => x.Channel)
                .Select(x => x == -1 ? "-" : x.ToString())
                .ToProperty(this, x => x.ChannelName);

            _type = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(drivers.IShutterDriver.Types.DISABLED) :
                    this.WhenAnyValue(x => x.Driver.Type))
                .Switch()
                .ToProperty(this, x => x.Type);
            _stateAtPowerUp = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(drivers.IShutterDriver.States.UNKNOWN) :
                    this.WhenAnyValue(x => x.Driver.StateAtPowerUp))
                .Switch()
                .ToProperty(this, x => x.StateAtPowerUp);
            _pulseVoltage = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(0d) :
                    this.WhenAnyValue(x => x.Driver, x => x.Driver.PulseDutyCyclePercentage)
                        .Select(x => x.Item1.DutyCycleToVoltage(x.Item2)))
                .Switch()
                .ToProperty(this, x => x.PulseVoltage);
            _holdVoltage = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(0d) :
                    this.WhenAnyValue(x => x.Driver, x => x.Driver.HoldDutyCyclePercentage)
                        .Select(x => x.Item1.DutyCycleToVoltage(x.Item2)))
                .Switch()
                .ToProperty(this, x => x.HoldVoltage);
            _pulseWidth = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(TimeSpan.Zero) :
                    this.WhenAnyValue(x => x.Driver.PulseWidth))
                .Switch()
                .ToProperty(this, x => x.PulseWidth);

            _pulseVoltageScratchpadEnabled = this
                .WhenAnyValue(x => x.Type)
                .Select(x => x != drivers.IShutterDriver.Types.DISABLED)
                .ToProperty(this, x => x.PulseVoltageScratchpadEnabled);
            _holdVoltageScratchpadEnabled = this
                .WhenAnyValue(x => x.Type)
                .Select(x => x == drivers.IShutterDriver.Types.BIDIRECTIONAL_PULSE_HOLD
                    || x == drivers.IShutterDriver.Types.UNIDIRECTIONAL_PULSE_HOLD)
                .ToProperty(this, x => x.HoldVoltageScratchpadEnabled);
            _pulseWidthScratchpadEnabled = this
                .WhenAnyValue(x => x.Type)
                .Select(x => x != drivers.IShutterDriver.Types.DISABLED)
                .ToProperty(this, x => x.PulseWidthScratchpadEnabled);

            CommitSettings = ReactiveCommand.CreateFromTask(
                (CancellationToken token) =>
                {
                    return Type switch
                    {
                        drivers.IShutterDriver.Types.DISABLED => Driver.ConfigureNoDriver(token),
                        drivers.IShutterDriver.Types.PULSED => Driver.ConfigurePulsed(
                            Driver.VoltageToDutyCycle(PulseVoltageScratchpad), PulseWidthScratchpad,
                            StateAtPowerUpScratchpad, token),
                        drivers.IShutterDriver.Types.BIDIRECTIONAL_PULSE_HOLD => Driver.ConfigureBidirectionalPulseHold(
                            Driver.VoltageToDutyCycle(PulseVoltageScratchpad), PulseWidthScratchpad,
                            Driver.VoltageToDutyCycle(HoldVoltageScratchpad), StateAtPowerUpScratchpad, token),
                        drivers.IShutterDriver.Types.UNIDIRECTIONAL_PULSE_HOLD => Driver.ConfigureUnidirectionalPulseHold(
                            Driver.VoltageToDutyCycle(PulseVoltageScratchpad), PulseWidthScratchpad,
                            Driver.VoltageToDutyCycle(HoldVoltageScratchpad), StateAtPowerUpScratchpad, token),
                        _ => throw new InvalidOperationException($"shutter type of {(int)Type:d} is not supported"),
                    };
                }, this.WhenAnyValue(x => x.Driver).Select(x => x is not null));

            RevertSettings = ReactiveCommand.Create(
                () =>
                {
                    TypeScratchpad = Type;
                    StateAtPowerUpScratchpad = StateAtPowerUp;
                    PulseVoltageScratchpad = PulseVoltage;
                    HoldVoltageScratchpad = HoldVoltage;
                    PulseWidthScratchpad = PulseWidth;
                }, this.WhenAnyValue(x => x.Driver).Select(x => x is not null));
        }




        private readonly ObservableAsPropertyHelper<int> _channel;
        private readonly ObservableAsPropertyHelper<string> _channelName;

        private readonly ObservableAsPropertyHelper<drivers.IShutterDriver.Types> _type;
        private readonly ObservableAsPropertyHelper<drivers.IShutterDriver.States> _stateAtPowerUp;
        private readonly ObservableAsPropertyHelper<double> _pulseVoltage;
        private readonly ObservableAsPropertyHelper<double> _holdVoltage;
        private readonly ObservableAsPropertyHelper<TimeSpan> _pulseWidth;



        private drivers.IShutterDriver.Types _typeScratchpad = drivers.IShutterDriver.Types.DISABLED;
        private drivers.IShutterDriver.States _stateAtPowerUpScratchpad = drivers.IShutterDriver.States.UNKNOWN;
        private double _pulseVoltageScratchpad = 0;
        private double _holdVoltageScratchpad = 0;
        private TimeSpan _pulseWidthScratchpad = TimeSpan.Zero;
        private readonly ObservableAsPropertyHelper<bool> _pulseVoltageScratchpadEnabled;
        private readonly ObservableAsPropertyHelper<bool> _holdVoltageScratchpadEnabled;
        private readonly ObservableAsPropertyHelper<bool> _pulseWidthScratchpadEnabled;

        private drivers.IShutterDriver _driver = null;
    }
}
