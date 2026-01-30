using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reactive.Linq;
using System.Reactive.Subjects;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Input;

namespace MCM6000_UI.view_models
{
    public class UsbUsageProfilerVM : ReactiveObject
    {
        private models.UsbUsageProfiler _model = null;
        public models.UsbUsageProfiler Model
        {
            get => _model;
            set => this.RaiseAndSetIfChanged(ref _model, value);
        }

        private DateTime _lastDump = DateTime.MinValue;
        private IEnumerable<UsbWriteProfileStatsVM> _profiles = [];
        private readonly ObservableAsPropertyHelper<IEnumerable<UsbWriteProfileStatsVM>> _filteredProfiles;
        public DateTime ProfileDumpTime
        {
            get => _lastDump;
            private set => this.RaiseAndSetIfChanged(ref _lastDump, value);
        }
        public IEnumerable<UsbWriteProfileStatsVM> Profiles
        {
            get => _profiles;
            private set
            {
                this.RaiseAndSetIfChanged(ref _profiles, value);
                ProfileDumpTime = DateTime.Now;
            }
        }
        public IEnumerable<UsbWriteProfileStatsVM> FilteredProfiles => _filteredProfiles.Value;


        private int _usageUpdateIntervalMs = 1000;
        public int UsageUpdateIntervalMs
        {
            get => _usageUpdateIntervalMs;
            set => this.RaiseAndSetIfChanged(ref _usageUpdateIntervalMs, value);
        }

        private int _usageWindowCount1 = 5;
        public int UsageWindowCount1
        {
            get => _usageWindowCount1;
            set
            {
                value = Math.Max(value, 1);
                this.RaiseAndSetIfChanged(ref _usageWindowCount1, value);
            }
        }

        private int _usageWindowCount2 = 10;
        public int UsageWindowCount2
        {
            get => _usageWindowCount2;
            set
            {
                value = Math.Max(value, 1);
                this.RaiseAndSetIfChanged(ref _usageWindowCount2, value);
            }
        }

        private bool _separateProfilesByThread = false;
        public bool AreProfilesSeparatedByThread
        {
            get => _separateProfilesByThread;
            set => this.RaiseAndSetIfChanged(ref _separateProfilesByThread, value);
        }

        public ICommand DumpProfiles
        { get; }

        private readonly ObservableAsPropertyHelper<double> _usageUpdateIntervalSeconds;
        private readonly ObservableAsPropertyHelper<double> _usageWindow1IntervalSeconds;
        private readonly ObservableAsPropertyHelper<double> _usageWindow2IntervalSeconds;
        public double UsageUpdateIntervalSeconds => _usageUpdateIntervalSeconds.Value;
        public double UsageWindow1IntervalSeconds => _usageWindow1IntervalSeconds.Value;
        public double UsageWindow2IntervalSeconds => _usageWindow2IntervalSeconds.Value;

        private readonly ObservableAsPropertyHelper<DateTime> _lastRecord;
        private readonly ObservableAsPropertyHelper<double> _usagePercentage;
        private readonly ObservableAsPropertyHelper<double> _usagePercentageWindow1;
        private readonly ObservableAsPropertyHelper<double> _usagePercentageWindow2;
        public DateTime UpdateStatisticsTimestamp => _lastRecord.Value;
        public double UsagePercentage => _usagePercentage.Value;
        public double UsagePercentageWindow1 => _usagePercentageWindow1.Value;
        public double UsagePercentageWindow2 => _usagePercentageWindow2.Value;


        public UsbUsageProfilerVM()
        {
            _usageUpdateIntervalSeconds = this
                .WhenAnyValue(x => x.UsageUpdateIntervalMs)
                .Select(x => (double)x / 1000)
                .ToProperty(this, x => x.UsageUpdateIntervalSeconds);
            _usageWindow1IntervalSeconds = this
                .WhenAnyValue(
                    x => x.UsageUpdateIntervalSeconds,
                    x => x.UsageWindowCount1, (l, r) => l * r)
                .ToProperty(this, x => x.UsageWindow1IntervalSeconds);
            _usageWindow2IntervalSeconds = this
                .WhenAnyValue(
                    x => x.UsageUpdateIntervalSeconds,
                    x => x.UsageWindowCount2, (l, r) => l * r)
                .ToProperty(this, x => x.UsageWindow2IntervalSeconds);

            var usageRecordObs = this
                .WhenAnyValue(x => x.Model)
                .Select(x => x?.UsageStats ?? Observable.Empty<models.UsagePercentageRecord>())
                .Switch();

            _lastRecord = usageRecordObs
                .Select(x => x.Timestamp)
                .Prepend(DateTime.Now)
                .ToProperty(this, x => x.UpdateStatisticsTimestamp);

            _filteredProfiles = this
                .WhenAnyValue(
                    x => x.Profiles,
                    x => x.AreProfilesSeparatedByThread,
                    (profiles, threadFilter) =>
                        profiles.Where(x => x.Name.Contains('@') == threadFilter).ToArray())
                .ToProperty(this, x => x.FilteredProfiles);

            var latestUsage = usageRecordObs
                .Select(x => x.UsagePercentage);

            _usagePercentage = latestUsage
                .Prepend(0)
                .ToProperty(this, x => x.UsagePercentage);

            _usagePercentageWindow1 = this
                .WhenAnyValue(x => x.UsageWindowCount1)
                .Select(x => latestUsage.Buffer(x)
                                        .Select(x => x.Average()))
                .Switch()
                .ToProperty(this, x => x.UsagePercentageWindow1);
            _usagePercentageWindow2 = this
                .WhenAnyValue(x => x.UsageWindowCount2)
                .Select(x => latestUsage.Buffer(x)
                                        .Select(x => x.Average()))
                .Switch()
                .ToProperty(this, x => x.UsagePercentageWindow2);

            DumpProfiles = ReactiveCommand.CreateFromTask(() => Task.Run(() => 
            {
                var models = Model.GetAllStats();
                var vms = models
                    .Select(x => new UsbWriteProfileStatsVM() { Model = x, });
                Profiles = vms;
            }), this.WhenAnyValue(x => x.Model).Select(x => x is not null));

            this.WhenAnyValue(x => x.Model)
                .WhereNotNull()
                .CombineLatest(this.WhenAnyValue(x => x.UsageUpdateIntervalMs))
                .Subscribe(tup =>
                {
                    var (model, periodMs) = tup;

                    model.SetUsageTimerUpdateInterval(TimeSpan.FromMilliseconds(periodMs));
                });
        }
    }
}
