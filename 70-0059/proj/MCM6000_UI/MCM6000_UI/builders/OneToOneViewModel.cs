using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MCM6000_UI.builders
{
    public class OneToOneViewModel : OneWireBuilderViewModel
    {
        #region Injected Properties
        public IEnumerable<string> PartTypes
        {
            get => _partNames;
            set => this.RaiseAndSetIfChanged(ref _partNames, value);
        }
        #endregion

        public string SelectedPartType
        {
            get
            {
                return (_selectedPartType is null || !PartTypes.Contains(_selectedPartType))
                      ? null : _selectedPartType;
                       
            }
            set
            {
                this.RaiseAndSetIfChanged(ref _selectedPartType, value);
                DeviceKey = value;
            }
        }

        public override void ConfigurePayload(byte[] buffer, int offset, int length)
        { RenamePayload(buffer, offset, length, DeviceKey); }


        public OneToOneViewModel()
            : base("1:1")
        {
            _subscriptions =
            [
                this.WhenAnyValue(x => x.PartTypes)
                    .Do(x =>
                    {
                        if (SelectedPartType is not null && !x.Contains(SelectedPartType))
                        {
                            SelectedPartType = null;
                        }
                    })
                    .Subscribe(),
                this.WhenAnyValue(x => x.SelectedPartType)
                    .Subscribe(x => {
                        IsReady = x is not null;
                    })
            ];
        }





        protected override void Dispose(bool manual)
        {
            base.Dispose(manual);
            if (_disposed)
                return;

            if (manual)
            {
                _partTypes.Dispose();

                foreach (var sub in _subscriptions)
                    sub.Dispose();
            }

            _disposed = true;
        }





        private readonly ObservableAsPropertyHelper<IEnumerable<string>> _partTypes;
        private readonly IEnumerable<IDisposable> _subscriptions;

        // PartType -> DeviceSignature
        private IEnumerable<string> _partNames = Array.Empty<string>();
        private string _selectedPartType = null;

        private bool _disposed = false;
    }
}
