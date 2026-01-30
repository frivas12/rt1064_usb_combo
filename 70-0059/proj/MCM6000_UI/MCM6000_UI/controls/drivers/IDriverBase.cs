using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MCM6000_UI.controls.drivers
{
    public interface IDriverBase : IReactiveObject
    {
        public int Channel
        { get; }
    }
}
