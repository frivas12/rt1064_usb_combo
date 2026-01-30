using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reactive.Disposables;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MCM6000_UI.builders
{
    public class OneWireSystemViewModel : OneWireBuilderViewModel
    {
        #region Injected Properties
        public Dictionary<string, IEnumerable<string>> SystemAndAxes
        {
            get => _systemAndAxes;
            set => this.RaiseAndSetIfChanged(ref _systemAndAxes, value);
        }
        #endregion

        public IEnumerable<string> SystemTypes => _systemTypes.Value;
        public IEnumerable<string> AxisTypes => _axisTypes.Value;

        public string SelectedSystem
        {
            get => _selectedSystem;
            set => this.RaiseAndSetIfChanged(ref _selectedSystem, value);
        }

        public string SelectedAxis
        {
            get => _selectedAxis;
            set => this.RaiseAndSetIfChanged(ref _selectedAxis, value);
        }

        public bool IsSystemReady =>
            _isSystemReady.Value;

        public bool IsAxisReady =>
            _isAxisReady.Value;

        public override void ConfigurePayload(byte[] buffer, int offset, int length) =>
            RenamePayload(buffer, offset, length, DeviceKey.Replace('/', '-'));

        public OneWireSystemViewModel()
            : base("system")
        {
            var isSystemProvided = this.WhenAnyValue(x => x.SelectedSystem)
                .Select(x => x is not null);

            var isAxisProvided = this.WhenAnyValue(x => x.SelectedAxis)
                .Select(x => x is not null);

            _isSystemReady = isSystemProvided.ToProperty(this, x => x.IsSystemReady);
            _isAxisReady = isAxisProvided.ToProperty(this, x => x.IsAxisReady);


            _systemTypes = this.WhenAnyValue(x => x.SystemAndAxes)
                .Select(x => x.Keys)
                .ToProperty(this, x => x.SystemTypes);

            _axisTypes = this.WhenAnyValue(x => x.SystemAndAxes, x => x.SelectedSystem)
                .Select(tup =>
                {
                    var (dict, selection) = tup;

                    if (selection is null)
                    { return []; }

                    if (!dict.TryGetValue(selection, out var list))
                    { return []; }

                    return list;
                })
                .ToProperty(this, x => x.AxisTypes);

            _subscriptions = new CompositeDisposable(
                // When the injected property changes, reset the selections.
                this.WhenAnyValue(x => x.SystemAndAxes).Subscribe(_ => {
                    SelectedSystem = null;
                    SelectedAxis = null;
                }),

                // When the selected system changes, reset the selected axis.
                this.WhenAnyValue(x => x.SelectedSystem).Subscribe(_ => {
                    SelectedAxis = null;
                }),

                // DeviceKey
                this.WhenAnyValue(x => x.SelectedSystem, x => x.SelectedAxis)
                    .Subscribe(tup => DeviceKey = tup.Item1 + '/' + tup.Item2),

                // IsReady
                Observable.CombineLatest(isSystemProvided, isAxisProvided)
                    .Subscribe(x => IsReady = x[0] && x[1])
            );
        }





        protected override void Dispose(bool manual)
        {
            if (_disposed)
                return;

            if (manual)
            {
                _isSystemReady.Dispose();
                _isAxisReady.Dispose();
                _systemTypes.Dispose();
                _axisTypes.Dispose();
                _subscriptions.Dispose();
            }

            _disposed = true;
        }





        private readonly ObservableAsPropertyHelper<bool> _isSystemReady;
        private readonly ObservableAsPropertyHelper<bool> _isAxisReady;
        private readonly ObservableAsPropertyHelper<IEnumerable<string>> _systemTypes;
        private readonly ObservableAsPropertyHelper<IEnumerable<string>> _axisTypes;
        private readonly IDisposable _subscriptions;

        private Dictionary<string, IEnumerable<string>> _systemAndAxes = [];
        private string _selectedSystem = null;
        private string _selectedAxis = null;

        private bool _disposed = false;
    }
}
