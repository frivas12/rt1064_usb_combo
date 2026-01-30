using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MCM6000_UI.view_models
{
    public class UsbWriteProfileStatsVM : ReactiveObject
    {
        private models.UsbWriteProfileStats _model = null;
        public models.UsbWriteProfileStats Model
        {
            get => _model;
            set => this.RaiseAndSetIfChanged(ref _model, value);
        }

        public string Name => _name.Value;
        public int BlockingCalls => _blockingCalls.Value;
        public int TimeoutCalls => _timeoutCalls.Value;
        public int SuccessfulCalls => _successfulCalls.Value;
        public int FailedCalls => _failedCalls.Value;
        public ulong TotalBytesTransferred => _totalBytesTransferred.Value;
        public TimeSpan TotalTimeBlockingOnMutexWithoutTimeout => _totalTimeBlockingOnMutexWithoutTimeout.Value;
        public TimeSpan TotalTimeBlockingOnMutexWithTimeout => _totalTimeBlockingOnMutexWithTimeout.Value;
        public TimeSpan TotalTimeBlockingOnCooldown => _totalTimeBlockingOnCooldown.Value;
        public TimeSpan TotalTimeInSuccessfulCalls => _totalTimeInSuccessfulCalls.Value;
        public TimeSpan TotalTimeInFailedCalls => _totalTimeInFailedCalls.Value;

        public int TotalCalls => _totalCalls.Value;
        public int ExecutionCalls => _executionCalls.Value;
        public int NonExecutionCalls => _nonExecutionCalls.Value;

        public TimeSpan TotalMutexBlockingTime => _totalMutexBlockingTime.Value;
        public TimeSpan TotalBlockingTime => _totalBlockingTime.Value;

        public double AverageBytesTransferred => _averageBytesTransferred.Value;
        public TimeSpan AverageMutexBlockingTime => _averageMutexBlockingTime.Value;
        public TimeSpan AverageBlockingTime => _averageBlockingTime.Value;




        public UsbWriteProfileStatsVM()
        {
            var statObs = this.WhenAnyValue(x => x.Model)
                //.Throttle(TimeSpan.FromSeconds(0.25))
                .Prepend(null);

            _name = statObs
                .Select(x => x?.Name ?? string.Empty)
                .ToProperty(this, x => x.Name);

            _blockingCalls = statObs
                .Select(x => x?.BlockingCalls ?? 0)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.BlockingCalls);

            _timeoutCalls = statObs
                .Select(x => x?.TimeoutCalls ?? 0)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.TimeoutCalls);

            _successfulCalls = statObs
                .Select(x => x?.SuccessfulCalls ?? 0)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.SuccessfulCalls);

            _failedCalls = statObs
                .Select(x => x?.FailedCalls ?? 0)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.FailedCalls);

            _totalBytesTransferred = statObs
                .Select(x => x?.TotalBytesTransferred ?? 0)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.TotalBytesTransferred);

            _totalTimeBlockingOnMutexWithoutTimeout = statObs
                .Select(x => x?.TotalTimeBlockingOnMutexWithoutTimeout ?? TimeSpan.Zero)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.TotalTimeBlockingOnMutexWithoutTimeout);

            _totalTimeBlockingOnMutexWithTimeout = statObs
                .Select(x => x?.TotalTimeBlockingOnMutexWithTimeout ?? TimeSpan.Zero)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.TotalTimeBlockingOnMutexWithTimeout);

            _totalTimeBlockingOnCooldown = statObs
                .Select(x => x?.TotalTimeBlockingOnCooldown ?? TimeSpan.Zero)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.TotalTimeBlockingOnCooldown);

            _totalTimeInSuccessfulCalls = statObs
                .Select(x => x?.TotalTimeInSuccessfulCalls ?? TimeSpan.Zero)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.TotalTimeInSuccessfulCalls);

            _totalTimeInFailedCalls = statObs
                .Select(x => x?.TotalTimeInFailedCalls ?? TimeSpan.Zero)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.TotalTimeInFailedCalls);

            _totalCalls = this.WhenAnyValue(
                    x => x.BlockingCalls,
                    x => x.TimeoutCalls,
                    (l, r) => l + r)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.TotalCalls);

            _executionCalls = this.WhenAnyValue(
                    x => x.SuccessfulCalls,
                    x => x.FailedCalls,
                    (l, r) => l + r)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.ExecutionCalls);

            _nonExecutionCalls = this.WhenAnyValue(
                    x => x.TotalCalls,
                    x => x.ExecutionCalls,
                    (l, r) => l - r)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.NonExecutionCalls);

            _averageBytesTransferred = this.WhenAnyValue(
                    x => x.TotalBytesTransferred,
                    x => x.SuccessfulCalls,
                    (l, r) => (double)l / r)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.AverageBytesTransferred);

            _totalMutexBlockingTime = this.WhenAnyValue(
                    x => x.TotalTimeBlockingOnMutexWithoutTimeout,
                    x => x.TotalTimeBlockingOnMutexWithTimeout,
                    (l, r) => l + r)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.TotalMutexBlockingTime);

            _totalBlockingTime = this.WhenAnyValue(
                    x => x.TotalMutexBlockingTime,
                    x => x.TotalTimeBlockingOnCooldown,
                    (l, r) => l + r)
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.TotalBlockingTime);

            _averageMutexBlockingTime = this.WhenAnyValue(
                    x => x.TotalMutexBlockingTime,
                    x => x.TotalCalls,
                    (l, r) => { return r == 0 ? TimeSpan.Zero : TimeSpan.FromTicks(l.Ticks / r); })
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.TotalMutexBlockingTime);

            _averageBlockingTime = this.WhenAnyValue(
                    x => x.TotalBlockingTime,
                    x => x.TotalCalls,
                    (l, r) => { return r == 0 ? TimeSpan.Zero : TimeSpan.FromTicks(l.Ticks / r); })
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToProperty(this, x => x.TotalBlockingTime);
        }


        private readonly ObservableAsPropertyHelper<string> _name;
        private readonly ObservableAsPropertyHelper<int> _blockingCalls;
        private readonly ObservableAsPropertyHelper<int> _timeoutCalls;
        private readonly ObservableAsPropertyHelper<int> _successfulCalls;
        private readonly ObservableAsPropertyHelper<int> _failedCalls;
        private readonly ObservableAsPropertyHelper<ulong> _totalBytesTransferred;
        private readonly ObservableAsPropertyHelper<TimeSpan> _totalTimeBlockingOnMutexWithoutTimeout;
        private readonly ObservableAsPropertyHelper<TimeSpan> _totalTimeBlockingOnMutexWithTimeout;
        private readonly ObservableAsPropertyHelper<TimeSpan> _totalTimeBlockingOnCooldown;
        private readonly ObservableAsPropertyHelper<TimeSpan> _totalTimeInSuccessfulCalls;
        private readonly ObservableAsPropertyHelper<TimeSpan> _totalTimeInFailedCalls;

        private readonly ObservableAsPropertyHelper<int> _totalCalls;
        private readonly ObservableAsPropertyHelper<int > _executionCalls;
        private readonly ObservableAsPropertyHelper<int > _nonExecutionCalls;

        private readonly ObservableAsPropertyHelper<TimeSpan> _totalMutexBlockingTime;
        private readonly ObservableAsPropertyHelper<TimeSpan> _totalBlockingTime;

        private readonly ObservableAsPropertyHelper<double> _averageBytesTransferred;
        private readonly ObservableAsPropertyHelper<TimeSpan> _averageMutexBlockingTime;
        private readonly ObservableAsPropertyHelper<TimeSpan> _averageBlockingTime;
    }
}
