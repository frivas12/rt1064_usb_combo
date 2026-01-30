using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace MCM6000_UI.models
{
    public class UsbUsageProfiler : IDisposable
    {
        public class Profiler
        {
            public struct Stats
            {
                public string Profile;
                public Thread CallingThread;

                public readonly string ThreadProfile => ToThreadProfile(Profile, CallingThread);

                public DateTime MutexWaitStart;
                public DateTime MutexWaitEnd;
                public readonly DateTime CooldownStart => MutexWaitEnd;
                public DateTime CooldownEnd;
                public readonly DateTime ExecutionStart => CooldownEnd;
                public DateTime ExecutionEnd;

                public bool ObtainedMutex;
                public bool TimeoutOnMutex;
                public bool ExecutedSuccessfully;
                public int BytesToTransfer;

                public static string ToThreadProfile(string profile, Thread callingThread)
                {
                    string ext = callingThread.Name;
                    if (ext is null || !ext.Any())
                    {
                        ext = $"0x{callingThread.ManagedThreadId:X}";
                    }

                    return profile + "@" + ext;
                }

                public static Tuple<string, string> SeparateProfile(string threadProfile)
                {
                    var split = threadProfile.Split('@');
                    return Tuple.Create(split[0], split.ElementAtOrDefault(1) ?? string.Empty);
                }
            }

            public static Profiler Empty => _empty;

            public void MarkBlockedWaiting()
            {
                _stats.MutexWaitStart = DateTime.Now;
                _stats.TimeoutOnMutex = false;
            }
            public void MarkTimeoutWaiting()
            {
                _stats.MutexWaitStart = DateTime.Now;
                _stats.TimeoutOnMutex = true;
            }
            public void MarkMutexObtained()
            {
                _stats.MutexWaitEnd = DateTime.Now;
                _stats.ObtainedMutex = true;
            }
            public void MarkMutexAborted()
            {
                _stats.MutexWaitEnd = DateTime.Now;
                _stats.ObtainedMutex = false;
            }
            public void MarkCooldownPassed()
            {
                _stats.CooldownEnd = DateTime.Now;
            }

            public void MarkStartTransfer(int length)
            {
                _stats.BytesToTransfer = length;
            }

            public void FinishTransferSuccessfully()
            {
                _stats.ExecutionEnd = DateTime.Now;
                _stats.ExecutedSuccessfully = true;
            }
            public void FinishTransferFailure()
            {
                _stats.ExecutionEnd = DateTime.Now;
                _stats.ExecutedSuccessfully = false;
            }

            public void Commit()
            {
                _receiver?.Invoke(_stats);
            }



            public Profiler(string profile, Action<Stats> receiver)
            {
                if (profile.Contains('@'))
                {
                    throw new ArgumentException("profiles cannot contain the '@' character");
                }
                _receiver = receiver;
                _stats = new()
                {
                    Profile = profile,
                    CallingThread = Thread.CurrentThread,
                };
            }

            private readonly static Profiler _empty = new(string.Empty, null);


            private readonly Action<Stats> _receiver;
            private Stats _stats;
        }

        public class CapacityMonitor
        {
            public void MarkInUse()
            {
                var now = DateTime.Now;
                lock (_lock)
                {
                    if (_inUse)
                        return;
                    _inUse = true;
                    _timeNotInUse += now - _startTime;
                    _startTime = now;
                }
            }

            public void MarkNotInUse()
            {
                var now = DateTime.Now;
                lock (_lock)
                {
                    if (!_inUse)
                        return;
                    _inUse = false;
                    _timeInUse += now - _startTime;
                    _startTime = now;
                }
            }

            public UsagePercentageRecord GetStats()
            {
                lock(_lock)
                { return GetStatsNoLock(_running); }
            }

            public void Start(bool inUse = false)
            {
                lock (_lock)
                {
                    _timeInUse = TimeSpan.Zero;
                    _timeNotInUse = TimeSpan.Zero;
                    _startTime = DateTime.Now;
                    _running = true;
                    _inUse = inUse;
                }
            }

            public UsagePercentageRecord Stop()
            {
                lock (_lock)
                {
                    var delta = DateTime.Now - _startTime;
                    _running = false;
                    if (_inUse)
                    {
                        _timeInUse += delta;
                    }
                    else
                    {
                        _timeNotInUse += delta;
                    }

                    return GetStatsNoLock(false);
                }
            }

            public UsagePercentageRecord Reset()
            {
                lock (_lock)
                {
                    var rt = GetStatsNoLock(true);
                    _timeInUse = TimeSpan.Zero;
                    _timeNotInUse = TimeSpan.Zero;
                    _startTime = DateTime.Now;
                    _running = true;
                    return rt;
                }
            }

            private readonly object _lock = new();
            private TimeSpan _timeInUse = TimeSpan.Zero;
            private TimeSpan _timeNotInUse = TimeSpan.Zero;

            private bool _running = false;
            private bool _inUse = false;
            private DateTime _startTime;

            private UsagePercentageRecord GetStatsNoLock(bool running)
            {
                var now = DateTime.Now;
                TimeSpan inUse = _timeInUse;
                TimeSpan notInUse = _timeNotInUse;
                if (running)
                {
                    if (_inUse)
                    {
                        inUse += now - _startTime;
                    }
                    else
                    {
                        notInUse += now - _startTime;
                    }
                }

                return new UsagePercentageRecord()
                {
                    Timestamp = now,
                    TimeInUse = inUse,
                    TimeNotInUse = notInUse,
                };
            }
        }

        public IObservable<UsagePercentageRecord> UsageStats => _lastUsageStat;

        public Profiler BeginProfiling(string profile)
        {
            return new(profile, ReceiveProfilerStats);
        }

        public void MarkUsbInUse() => _capacityMonitor.MarkInUse();

        public void MarkUsbNotInUse() => _capacityMonitor.MarkNotInUse();

        public IEnumerable<UsbWriteProfileStats> GetAllStats()
        {
            lock (_profilesLock)
            {
                var rt = new UsbWriteProfileStats[_profiles.Count];

                int index = 0;
                foreach (var tup in _profiles.Values)
                {
                    lock (tup.Item2)
                    {
                        rt[index] = new UsbWriteProfileStats(tup.Item1);
                    }
                    ++index;
                }

                return rt;
            }
        }
        public UsbWriteProfileStats GetStats(string profile, Thread runningOnThread) =>
            GetStats(Profiler.Stats.ToThreadProfile(profile, runningOnThread));
        public UsbWriteProfileStats GetStats(string profile)
        {
            var tup = GetStatTuple(profile);
            lock (tup.Item2)
            {
                return new(tup.Item1);
            }
        }

        public TimeSpan GetUsageTimerUpdateInterval() => TimeSpan.FromMilliseconds(_capacityAnalysisDurationMs);
        public void SetUsageTimerUpdateInterval(TimeSpan period)
        {
            if (period > TimeSpan.FromMilliseconds(int.MaxValue))
            {
                throw new ArgumentException("period is too large");
            }
            _capacityAnalysisDurationMs = (int)period.TotalMilliseconds;
            _capacityAnalysisDurationMs = Math.Max(_capacityAnalysisDurationMs, 10);
            SetupTimer();
        }

        public UsbUsageProfiler()
        {
            _statProcessingThread = new Thread(new ParameterizedThreadStart(StatProcessingThread));
            _statProcessingThread.Start(this);

            _capacityMonitor = new();
            _capacityMonitor.Start();

            SetupTimer();
        }

        public void Dispose()
        {
            if (_disposed)
                return;

            _killThread = true;
            _eventsInQueue.Release();
            _statProcessingThread.Join();
            _eventsInQueue.Dispose();
            _profileUpdated.Dispose();

            _capacityMonitor.Stop();
            _lastUsageStat.Dispose();
            _capacityTimer?.Dispose();

            _disposed = true;
        }


        private bool _disposed = false;

        private readonly SemaphoreSlim _eventsInQueue = new(0);
        private readonly ConcurrentQueue<Profiler.Stats> _profilerValues = new();
        private readonly Thread _statProcessingThread;
        private bool _killThread = false;
        private readonly System.Reactive.Subjects.Subject<string> _profileUpdated = new();
        
        private readonly object _profilesLock = new();
        private readonly Dictionary<string, Tuple<UsbWriteProfileStats, object>> _profiles = [];

        private readonly CapacityMonitor _capacityMonitor;
        private readonly System.Reactive.Subjects.Subject<UsagePercentageRecord> _lastUsageStat = new();
        private Timer _capacityTimer;
        private int _capacityAnalysisDurationMs = 1000;


        private Tuple<UsbWriteProfileStats, object> GetStatTuple(string profile)
        {
            lock(_profilesLock)
            {
                if (!_profiles.TryGetValue(profile, out var tup))
                {
                    tup = Tuple.Create(new UsbWriteProfileStats(profile), new object());

                    _profiles[profile] = tup;
                }

                return tup;
            }
        }
        private static void StatProcessingThread(object obj)
        {
            var self = (UsbUsageProfiler)obj;
            self.StatProcessingThread();
        }
        private void StatProcessingThread()
        {
            while (true)
            {
                _eventsInQueue.Wait();
                if (_killThread)
                { break; }

                if (!_profilerValues.TryDequeue(out Profiler.Stats toProcess))
                { continue; }


                string threadedProfile = toProcess.ThreadProfile;
                var threadedTuple = GetStatTuple(threadedProfile);
                var aggregateTuple = GetStatTuple(toProcess.Profile);
                UsbWriteProfileStats threadClone;
                UsbWriteProfileStats aggregateClone;

                // Process Stats into Thread
                lock (threadedTuple.Item2)
                {
                    var mutexWait = toProcess.MutexWaitEnd - toProcess.MutexWaitStart;
                    if (toProcess.TimeoutOnMutex)
                    {
                        ++threadedTuple.Item1.TimeoutCalls;
                        threadedTuple.Item1.TotalTimeBlockingOnMutexWithTimeout += mutexWait;
                    }
                    else
                    {
                        ++threadedTuple.Item1.BlockingCalls;
                        threadedTuple.Item1.TotalTimeBlockingOnMutexWithoutTimeout += mutexWait;
                    }

                    if (toProcess.ObtainedMutex)
                    {
                        var cooldownWait = toProcess.CooldownEnd - toProcess.CooldownStart;
                        var executionTime = toProcess.ExecutionEnd - toProcess.ExecutionStart;
                        threadedTuple.Item1.TotalTimeBlockingOnCooldown += cooldownWait;
                        if (toProcess.ExecutedSuccessfully)
                        {
                            ++threadedTuple.Item1.SuccessfulCalls;
                            threadedTuple.Item1.TotalTimeInSuccessfulCalls += executionTime;
                        }
                        else
                        {
                            ++threadedTuple.Item1.FailedCalls;
                            threadedTuple.Item1.TotalTimeInFailedCalls += executionTime;
                        }
                    }

                    threadClone = new(threadedTuple.Item1);
                }

                // Process thread stats into aggregate
                lock (aggregateTuple.Item2)
                {
                    var rt = aggregateTuple.Item1;

                    rt.BlockingCalls += threadClone.BlockingCalls;
                    rt.TimeoutCalls += threadClone.TimeoutCalls;

                    rt.SuccessfulCalls += threadClone.SuccessfulCalls;
                    rt.FailedCalls += threadClone.FailedCalls;

                    rt.TotalTimeBlockingOnCooldown += threadClone.TotalTimeBlockingOnCooldown;
                    rt.TotalTimeBlockingOnMutexWithoutTimeout += threadClone.TotalTimeBlockingOnMutexWithoutTimeout;
                    rt.TotalTimeBlockingOnMutexWithTimeout += threadClone.TotalTimeBlockingOnMutexWithTimeout;

                    rt.TotalTimeInFailedCalls += threadClone.TotalTimeInFailedCalls;
                    rt.TotalTimeInSuccessfulCalls += threadClone.TotalTimeInSuccessfulCalls;

                    aggregateClone = new(rt);
                }

                _profileUpdated.OnNext(threadedProfile);
                _profileUpdated.OnNext(toProcess.Profile);
            }
        }
        private void ReceiveProfilerStats (Profiler.Stats stats)
        {
            _profilerValues.Enqueue(stats);
            _eventsInQueue.Release();
        }

        private static void OnCapacityTimerTick(object obj)
        {
            UsbUsageProfiler self = (UsbUsageProfiler)obj;

            var stats = self._capacityMonitor.Reset();
            self._lastUsageStat.OnNext(stats);
        }
        private void SetupTimer()
        {
            _capacityTimer?.Dispose();

            _capacityTimer = new System.Threading.Timer(OnCapacityTimerTick, this,
                (int)_capacityAnalysisDurationMs,
                (int)_capacityAnalysisDurationMs);
            _capacityMonitor.Start();
        }
    }
}
