using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reactive;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace MCM6000_UI.controls.drivers
{
    public interface ITTLOutputDriver : IDriverBase
    {
        public bool IsEnabled
        { get; }

        public bool Level
        { get; }

        public Task SetEnableAsync(bool enable, CancellationToken token);
        public Task SetLevelAsync(bool level, CancellationToken token);
    }
}
