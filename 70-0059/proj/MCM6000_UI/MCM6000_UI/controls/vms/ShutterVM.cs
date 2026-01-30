using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;

namespace MCM6000_UI.controls.vms
{
    public class ShutterVM : ReactiveObject
    {
        public int Channel => _channel.Value;
        public string ChannelName => _channelName.Value;
        public IEnumerable<drivers.IShutterDriver.TriggerModes> AvailableTriggerModes => _availableTriggerModes.Value;

        public bool IsEnabled => _isEnabled.Value;
        public drivers.IShutterDriver.States State => _state.Value;
        public drivers.IShutterDriver.TriggerModes ExternalTriggerMode
        {
            get => _externalTriggerMode.Value;
            set => SetTrigger.Execute(value).Subscribe();
        }

        public drivers.IShutterDriver.Types Type => _type.Value;
        public drivers.IShutterDriver.States StateAtPowerUp => _stateAtPowerUp.Value;
        public double PulseVoltage => _pulseVoltage.Value;
        public double HoldVoltage => _holdVoltage.Value;
        public TimeSpan PulseWidth => _pulseWidth.Value;


        public ReactiveCommand<bool, Unit> SetChannelEnable
        { get; }
        public ICommand EnableChannel
        { get; }
        public ICommand DisableChannel
        { get; }
        public ICommand ToggleEnable
        { get; }


        public ReactiveCommand<drivers.IShutterDriver.States, Unit> MoveTo
        { get; }
        public ICommand MoveToOpen
        { get; }
        public ICommand MoveToClosed
        { get; }


        public ReactiveCommand<drivers.IShutterDriver.TriggerModes, Unit> SetTrigger
        { get; }
        public ICommand DisableTrigger
        { get; }
        public ICommand EnableTrigger
        { get; }
        public ICommand EnableInvertedTrigger
        { get; }



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


        public ShutterVM()
        {
            ICommand BindCommand<TParam>(ReactiveCommand<TParam, Unit> command, TParam value) =>
                ReactiveCommand.CreateFromObservable(() => command.Execute(value), command.CanExecute);
            ICommand BindCommandRuntimeParameter<TParam>(ReactiveCommand<TParam, Unit> command, Func<TParam> runtimeEvaluator) =>
                ReactiveCommand.CreateFromObservable(() => command.Execute(runtimeEvaluator()), command.CanExecute);

            _channel = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(-1) :
                    this.WhenAnyValue(x => x.Driver.Channel))
                .Switch()
                .ToProperty(this, x => x.Channel);
            _channelName = this
                .WhenAnyValue(x => x.Channel)
                .Select(x => x == -1 ? "--" : x.ToString())
                .ToProperty(this, x => x.ChannelName);
            _availableTriggerModes = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    var empty = new drivers.IShutterDriver.TriggerModes[] { drivers.IShutterDriver.TriggerModes.DISABLED };
                    var full = Enum.GetValues(typeof(drivers.IShutterDriver.TriggerModes)).Cast<drivers.IShutterDriver.TriggerModes>();

                    return x is null ? empty : full;
                })
                .ToProperty(this, x => x.AvailableTriggerModes);

            _isEnabled = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(false) :
                    this.WhenAnyValue(x => x.Driver.IsEnabled))
                .Switch()
                .ToProperty(this, x => x.IsEnabled);

            _state = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(drivers.IShutterDriver.States.UNKNOWN) :
                    this.WhenAnyValue(x => x.Driver.State))
                .Switch()
                .ToProperty(this, x => x.State);

            _externalTriggerMode = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(drivers.IShutterDriver.TriggerModes.DISABLED) :
                    this.WhenAnyValue(x => x.Driver.ExternalTriggerMode))
                .Switch()
                .ToProperty(this, x => x.ExternalTriggerMode);

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

            SetChannelEnable = ReactiveCommand.CreateFromTask(
                    (bool enable, CancellationToken token) => Driver.SetEnabledAsync(enable, token),
                    this.WhenAnyValue(x => x.Driver).Select(x => x is not null));
            EnableChannel = BindCommand(SetChannelEnable, true);
            DisableChannel = BindCommand(SetChannelEnable, false);
            ToggleEnable = BindCommandRuntimeParameter(SetChannelEnable, () => !IsEnabled);

            MoveTo = ReactiveCommand.CreateFromTask(
                    (drivers.IShutterDriver.States state, CancellationToken token) => Driver.SetStateAsync(state, token),
                    Observable.CombineLatest(
                        this.WhenAnyValue(x => x.Driver).Select(x => x is not null),
                        this.WhenAnyValue(x => x.ExternalTriggerMode).Select(x => x == drivers.IShutterDriver.TriggerModes.DISABLED)
                    ).Select(x => x.All(y => y)));
            MoveToOpen = BindCommand(MoveTo, drivers.IShutterDriver.States.OPEN);
            MoveToClosed = BindCommand(MoveTo, drivers.IShutterDriver.States.CLOSED);
            

            SetTrigger = ReactiveCommand.CreateFromTask(
                    (drivers.IShutterDriver.TriggerModes mode, CancellationToken token) => Driver.SetExternalTriggerModeAsync(mode, token),
                    this.WhenAnyValue(x => x.Driver).Select(x => x is not null));
            DisableTrigger = BindCommand(SetTrigger, drivers.IShutterDriver.TriggerModes.DISABLED);
            EnableTrigger = BindCommand(SetTrigger, drivers.IShutterDriver.TriggerModes.ENABLED);
            EnableInvertedTrigger = BindCommand(SetTrigger, drivers.IShutterDriver.TriggerModes.INVERTED);


            CommitSettings = ReactiveCommand.CreateFromTask(
                (CancellationToken token) =>
                {
                    return Type switch
                    {
                        drivers.IShutterDriver.Types.DISABLED => Driver.ConfigureNoDriverAsync(token),
                        drivers.IShutterDriver.Types.PULSED => Driver.ConfigurePulsedAsync(
                            Driver.VoltageToDutyCycle(PulseVoltageScratchpad), PulseWidthScratchpad,
                            StateAtPowerUpScratchpad, token),
                        drivers.IShutterDriver.Types.BIDIRECTIONAL_PULSE_HOLD => Driver.ConfigureBidirectionalPulseHoldAsync(
                            Driver.VoltageToDutyCycle(PulseVoltageScratchpad), PulseWidthScratchpad,
                            Driver.VoltageToDutyCycle(HoldVoltageScratchpad), StateAtPowerUpScratchpad, token),
                        drivers.IShutterDriver.Types.UNIDIRECTIONAL_PULSE_HOLD => Driver.ConfigureUnidirectionalPulseHoldAsync(
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




        private drivers.IShutterDriver _driver = null;



        private readonly ObservableAsPropertyHelper<int> _channel;
        private readonly ObservableAsPropertyHelper<string> _channelName;
        private readonly ObservableAsPropertyHelper<IEnumerable<drivers.IShutterDriver.TriggerModes>> _availableTriggerModes;

        private readonly ObservableAsPropertyHelper<bool> _isEnabled;
        private readonly ObservableAsPropertyHelper<drivers.IShutterDriver.States> _state;
        private readonly ObservableAsPropertyHelper<drivers.IShutterDriver.TriggerModes> _externalTriggerMode;

        private readonly ObservableAsPropertyHelper<drivers.IShutterDriver.Types> _type;
        private readonly ObservableAsPropertyHelper<drivers.IShutterDriver.States> _stateAtPowerUp;
        private readonly ObservableAsPropertyHelper<double> _pulseVoltage;
        private readonly ObservableAsPropertyHelper<double> _holdVoltage;
        private readonly ObservableAsPropertyHelper<TimeSpan> _pulseWidth;




        private readonly ObservableAsPropertyHelper<bool> _pulseVoltageScratchpadEnabled;
        private readonly ObservableAsPropertyHelper<bool> _holdVoltageScratchpadEnabled;
        private readonly ObservableAsPropertyHelper<bool> _pulseWidthScratchpadEnabled;

        private drivers.IShutterDriver.Types _typeScratchpad = drivers.IShutterDriver.Types.DISABLED;
        private drivers.IShutterDriver.States _stateAtPowerUpScratchpad = drivers.IShutterDriver.States.UNKNOWN;
        private double _pulseVoltageScratchpad = 0;
        private double _holdVoltageScratchpad = 0;
        private TimeSpan _pulseWidthScratchpad = TimeSpan.Zero;
    }
}
