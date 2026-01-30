using ReactiveUI;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace MCM6000_UI.builders
{
    public class OneWireV2BuilderViewModel : OneWireBuilderViewModel
    {
        #region Injected Properties
        public int SerialNumberLength
        {
            get => _serialNumberLength;
            set => this.RaiseAndSetIfChanged(ref _serialNumberLength, value);
        }

        public Func<string, bool> SerialNumberValidator
        {
            get => _serialNumberValidator;
            set => this.RaiseAndSetIfChanged(ref _serialNumberValidator, value);
        }

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
            set => this.RaiseAndSetIfChanged(ref _selectedPartType, value);
        }

        public string SerialNumber
        {
            get => _serialNumber;
            set => this.RaiseAndSetIfChanged(ref _serialNumber, value);
        }

        public bool IsTypeProvided =>
            _isSignatureProvided.Value;

        public bool IsSerialNumberProvided =>
            _isIdentifierProvided.Value;

        public override void ConfigurePayload(byte[] buffer, int offset, int length) =>
            RenamePayload(buffer, offset, length, $"{DeviceKey}-{SerialNumber}");


        public OneWireV2BuilderViewModel()
            : base("v2")
        {
            var isSignatureProvided = this.WhenAnyValue(x => x.SelectedPartType)
                                          .Select(x => x is not null);
            _isSignatureProvided = isSignatureProvided.ToProperty(this, x => x.IsTypeProvided);

            var isIdentifierProvided = this.WhenAnyValue(x => x.SerialNumber,
                                                      x => x.SerialNumberValidator)
                                        .Select(v =>
                                        {
                                            (var sn, var validator) = v;
                                            return validator(sn);
                                        });
            _isIdentifierProvided = isIdentifierProvided.ToProperty(this, x => x.IsSerialNumberProvided);

            _subscriptions =
            [
                this.WhenAnyValue(x => x.PartTypes)
                    .Subscribe(x =>
                    {
                        if (SelectedPartType is not null && !x.Contains(SelectedPartType))
                        {
                            SelectedPartType = null;
                        }
                    }),

                // DeviceKey
                this.WhenAnyValue(x => x.SelectedPartType)
                    .Subscribe(x => DeviceKey = x),

                // IsReady
                Observable.CombineLatest(isIdentifierProvided, isSignatureProvided)
                    .Subscribe(x => IsReady = x[0] && x[1]),
            ];
        }





        protected override void Dispose(bool manual)
        {
            base.Dispose(manual);
            if (_disposed)
                return;

            if (manual)
            {
                _isSignatureProvided.Dispose();
                _isIdentifierProvided.Dispose();
                _partTypes.Dispose();

                foreach (var sub in _subscriptions)
                    sub.Dispose();
            }

            _disposed = true;
        }





        private readonly ObservableAsPropertyHelper<bool> _isSignatureProvided;
        private readonly ObservableAsPropertyHelper<bool> _isIdentifierProvided;
        private readonly ObservableAsPropertyHelper<IEnumerable<string>> _partTypes;
        private readonly IEnumerable<IDisposable> _subscriptions;

        private Func<string, bool> _serialNumberValidator = _ => true;
        private int _serialNumberLength = int.MaxValue;
        // PartType -> DeviceSignature
        private IEnumerable<string> _partNames = Array.Empty<string>();
        private string _selectedPartType = null;
        private string _serialNumber = string.Empty;

        private bool _disposed = false;
    }
}
