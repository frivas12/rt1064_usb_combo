using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;

namespace MCM6000_UI.controls.vms
{
    public class ShutterControllerVM : ReactiveObject
    {
        public int Channel => _channel.Value;
        public string ChannelName => _channelName.Value;

        public bool IsEnabled => _isEnabled.Value;
        public drivers.IShutterDriver.States State => _state.Value;
        public drivers.IShutterDriver.TriggerModes ExternalTriggerMode => _externalTriggerMode.Value;


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



        public drivers.IShutterDriver Driver
        {
            get => _driver;
            set => this.RaiseAndSetIfChanged(ref _driver, value);
        }


        public ShutterControllerVM()
        {
            ICommand BindCommand<TParam>(ReactiveCommand<TParam, Unit> command, TParam value, IObservable<TParam> valueChecker) =>
                ReactiveCommand.CreateFromObservable(() => command.Execute(value), command.CanExecute.CombineLatest(valueChecker, (l, r) => l && !Equals(r, value)));
            ReactiveCommand<TParam, Unit> DecorateWithEnabled<TParam>(ReactiveCommand<TParam, Unit> command) =>
                ReactiveCommand.CreateFromObservable<TParam, Unit>(command.Execute, command.CanExecute.CombineLatest(this.WhenAnyValue(x => x.IsEnabled), (l, r) => l && r));

            _channel = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(-1) :
                    this.WhenAnyValue(x => x.Driver.Channel))
                .Switch()
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.Channel);
            _channelName = this
                .WhenAnyValue(x => x.Channel)
                .Select(x => x == -1 ? "-" : x.ToString())
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.ChannelName);

            _isEnabled = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(false) :
                    x.WhenAnyValue(y => y.IsEnabled))
                .Switch()
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.IsEnabled);

            _state = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(drivers.IShutterDriver.States.UNKNOWN) :
                    x.WhenAnyValue(y => y.State))
                .Switch()
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.State);

            _externalTriggerMode = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? Observable.Return(drivers.IShutterDriver.TriggerModes.DISABLED) :
                    x.WhenAnyValue(y => y.ExternalTriggerMode))
                .Switch()
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.ExternalTriggerMode);

            SetChannelEnable = ReactiveCommand.CreateFromTask(
                    (bool enable, CancellationToken token) => Driver.SetEnableAsync(enable, token),
                    this.WhenAnyValue(x => x.Driver).Select(x => x is not null));
            EnableChannel = BindCommand(SetChannelEnable, true, this.WhenAnyValue(x => x.IsEnabled));
            DisableChannel = BindCommand(SetChannelEnable, false, this.WhenAnyValue(x => x.IsEnabled));
            ToggleEnable = ReactiveCommand.CreateFromObservable(() => SetChannelEnable.Execute(!IsEnabled), SetChannelEnable.CanExecute);

            MoveTo = DecorateWithEnabled(ReactiveCommand.CreateFromTask(
                    (drivers.IShutterDriver.States state, CancellationToken token) => Driver.SetStateAsync(state, token),
                    this.WhenAnyValue(x => x.Driver).Select(x => x is not null)));
            MoveToOpen = BindCommand(MoveTo, drivers.IShutterDriver.States.OPEN, this.WhenAnyValue(x => x.State));
            MoveToClosed = BindCommand(MoveTo, drivers.IShutterDriver.States.CLOSED, this.WhenAnyValue(x => x.State));

            SetTrigger = DecorateWithEnabled(ReactiveCommand.CreateFromTask(
                    (drivers.IShutterDriver.TriggerModes mode, CancellationToken token) => Driver.SetExternalTriggerModeAsync(mode, token),
                    this.WhenAnyValue(x => x.Driver).Select(x => x is not null)));
            DisableTrigger = BindCommand(SetTrigger, drivers.IShutterDriver.TriggerModes.DISABLED, this.WhenAnyValue(x => x.ExternalTriggerMode));
            EnableTrigger = BindCommand(SetTrigger, drivers.IShutterDriver.TriggerModes.ENABLED, this.WhenAnyValue(x => x.ExternalTriggerMode));
            EnableInvertedTrigger = BindCommand(SetTrigger, drivers.IShutterDriver.TriggerModes.INVERTED, this.WhenAnyValue(x => x.ExternalTriggerMode));
        }




        private drivers.IShutterDriver _driver = null;



        private readonly ObservableAsPropertyHelper<int> _channel;
        private readonly ObservableAsPropertyHelper<string> _channelName;
        private readonly ObservableAsPropertyHelper<bool> _isEnabled;
        private readonly ObservableAsPropertyHelper<drivers.IShutterDriver.States> _state;
        private readonly ObservableAsPropertyHelper<drivers.IShutterDriver.TriggerModes> _externalTriggerMode;
    }
}
