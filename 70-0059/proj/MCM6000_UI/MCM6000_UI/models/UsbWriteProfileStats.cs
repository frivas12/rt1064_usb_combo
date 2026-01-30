using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MCM6000_UI.models
{
    [Serializable]
    public class UsbWriteProfileStats
    {
        public string Name { get; set; }

        public int BlockingCalls { get; set; }
        public int TimeoutCalls { get; set; }

        public int SuccessfulCalls { get; set; }
        public int FailedCalls { get; set; }

        public ulong TotalBytesTransferred { get; set; }

        public TimeSpan TotalTimeBlockingOnMutexWithoutTimeout;
        public TimeSpan TotalTimeBlockingOnMutexWithTimeout;
        public TimeSpan TotalTimeBlockingOnCooldown;

        public TimeSpan TotalTimeInSuccessfulCalls;
        public TimeSpan TotalTimeInFailedCalls;

        public double MutexBlockingNoTimeoutMs => TotalTimeBlockingOnMutexWithoutTimeout.TotalMilliseconds;
        public double MutexBlockingTimeoutMs => TotalTimeBlockingOnMutexWithTimeout.TotalMilliseconds;
        public double CooldownBlockingMs => TotalTimeBlockingOnCooldown.TotalMilliseconds;

        public double ExecutionTimeSuccessMs => TotalTimeInSuccessfulCalls.TotalMilliseconds;
        public double ExecutionTimeFailedMs => TotalTimeInFailedCalls.TotalMilliseconds;

        public UsbWriteProfileStats(string name)
        {
            Name = name;
            BlockingCalls = 0;
            FailedCalls = 0;
            SuccessfulCalls = 0;
            TimeoutCalls = 0;
            TotalTimeBlockingOnCooldown = TimeSpan.Zero;
            TotalTimeBlockingOnMutexWithoutTimeout = TimeSpan.Zero;
            TotalTimeBlockingOnMutexWithTimeout = TimeSpan.Zero;
            TotalTimeInFailedCalls = TimeSpan.Zero;
            TotalTimeInSuccessfulCalls = TimeSpan.Zero;
        }

        public UsbWriteProfileStats(UsbWriteProfileStats other)
        {
            Name = other.Name;
            BlockingCalls = other.BlockingCalls;
            TimeoutCalls = other.TimeoutCalls;

            SuccessfulCalls = other.SuccessfulCalls;
            FailedCalls = other.FailedCalls;

            TotalTimeBlockingOnCooldown = other.TotalTimeBlockingOnCooldown;
            TotalTimeBlockingOnMutexWithoutTimeout = other.TotalTimeBlockingOnMutexWithoutTimeout;
            TotalTimeBlockingOnMutexWithTimeout = other.TotalTimeBlockingOnMutexWithTimeout;

            TotalTimeInFailedCalls = other.TotalTimeInFailedCalls;
            TotalTimeInSuccessfulCalls = other.TotalTimeInSuccessfulCalls;
        }
    }
}
