using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reactive.Linq;
using System.Reactive.Subjects;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;

namespace MCM6000_UI.controls.vms
{
    public class TTLOutputVM : ReactiveObject
    {
        public int Channel => _channel.Value;
        public string ChannelName => _channelName.Value;

        public bool Level => _level.Value;

        public bool IsEnabled => _isEnabled.Value;

        public ICommand Enable
        { get; }
        public ICommand Disable
        { get; }
        public ICommand ToggleEnable => _toggleEnable.Value;

        public ICommand SetLevelLow
        { get; }
        public ICommand SetLevelHigh
        { get; }


        public drivers.ITTLOutputDriver Driver
        {
            get => _driver;
            set => this.RaiseAndSetIfChanged(ref _driver, value);
        }


        public TTLOutputVM()
        {
            var emptyChannelObs = new BehaviorSubject<int>(-1);
            var emptyStateObs = new BehaviorSubject<bool>(true);
            _channel = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null ? emptyChannelObs : this.WhenAnyValue(y => y.Driver.Channel))
                .Switch()
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.Channel);
            _channelName = this
                .WhenAnyValue(x => x.Channel)
                .Select(x => x == -1 ? "--" : x.ToString())
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.ChannelName);
            _level = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is null
                    ? emptyStateObs
                    : this.WhenAnyValue(y => y.Driver.Level))
                .Switch()
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.Level);
            _isEnabled = this
                .WhenAnyValue(x => x.Driver.IsEnabled)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.IsEnabled);

            Enable = ReactiveCommand.CreateFromTask(
                async _ =>
                {
                    await Driver?.SetEnableAsync(true, CancellationToken.None);
                },
                this.WhenAnyValue(x => x.IsEnabled).Select(x => !x));
            Disable = ReactiveCommand.CreateFromTask(
                async _ =>
                {
                    await Driver?.SetEnableAsync(false, CancellationToken.None);
                },
                this.WhenAnyValue(x => x.IsEnabled));
            _toggleEnable = this
                .WhenAnyValue(x => x.IsEnabled, x => x.Enable, x => x.Disable)
                .Select(tup =>
                {
                    var (enabled, enable, disable) = tup;
                    return enabled ? disable : enable;
                })
                .ToProperty(this, x => x.ToggleEnable);

            SetLevelHigh = ReactiveCommand.CreateFromTask(
                async _ =>
                {
                    await Driver?.SetLevelAsync(true, CancellationToken.None);
                },
                this.WhenAnyValue(x => x.IsEnabled).CombineLatest(this.WhenAnyValue(x => x.Level), (l, r) => l && !r));
            SetLevelLow = ReactiveCommand.CreateFromTask(
                async _ =>
                {
                    await Driver?.SetLevelAsync(false, CancellationToken.None);
                },
                this.WhenAnyValue(x => x.IsEnabled).CombineLatest(this.WhenAnyValue(x => x.Level), (l, r) => l && r));
        }


        private readonly ObservableAsPropertyHelper<int> _channel;
        private readonly ObservableAsPropertyHelper<string> _channelName;
        private readonly ObservableAsPropertyHelper<bool> _level;
        private readonly ObservableAsPropertyHelper<bool> _isEnabled;
        private readonly ObservableAsPropertyHelper<ICommand> _toggleEnable;

        private drivers.ITTLOutputDriver _driver = null;
    }
}
