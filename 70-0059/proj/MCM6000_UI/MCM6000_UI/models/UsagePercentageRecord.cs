using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MCM6000_UI.models
{
    public struct UsagePercentageRecord
    {
        public DateTime Timestamp;
        public TimeSpan TimeInUse;
        public TimeSpan TimeNotInUse;

        public readonly TimeSpan TotalTime => TimeInUse + TimeNotInUse;
        public readonly double UsagePercentage => 100d * (double)TimeInUse.Ticks / TotalTime.Ticks;
    }
}
