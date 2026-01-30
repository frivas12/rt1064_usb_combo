using MCM6000_UI.controls.drivers;
using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MCM6000_UI.controls.vms
{
    public class StepperVM : ReactiveObject
    {
        public IStepperDriver Driver
        {
            get => _driver;
            set => this.RaiseAndSetIfChanged(ref _driver, value);
        }

        public string EncoderPosition => _encoderPosition.Value;
        public string EncoderToolTip => _encoderToolTip.Value;
        public string StepperPosition => _stepperPosition.Value;

        public bool MovingCCW => _movingCCW.Value;
        public bool MovingCW => _movingCW.Value;
        public bool HardLimitCCW => _hardLimitCCW.Value;
        public bool HardLimitCW => _hardLimitCW.Value;
        public bool SoftLimitCCW => _softLimitCCW.Value;
        public bool SoftLimitCW => _softLimitCW.Value;

        public bool NotHomed => _notHomed.Value;
        public bool Homing => _homing.Value;
        public bool Homed => _homed.Value;

        public bool IsDeviceConnected => _isDeviceConnected.Value;

        public IStepperDriver.ISavedPosition CurrentSavedPosition => _currentSavedPosition.Value;

        public SavedPositionsEditorVM SavedPositionsEditor
        { get; } = new();

        public IEnumerable<IStepperDriver.ISavedPosition> SavedPositions;

        public byte FastMovePercentage
        {
            get => _fastMovePercentage;
            set
            {
                value = Math.Min((byte)100, Math.Max((byte)0, value));
                this.RaiseAndSetIfChanged(ref _fastMovePercentage, value);

                if (SlowMovePercentage > value)
                {
                    SlowMovePercentage = value;
                }
            }
        }
        public byte SlowMovePercentage
        {
            get => _slowMovePercentage;
            set
            {
                value = Math.Min((byte)100, Math.Max((byte)0, value));
                this.RaiseAndSetIfChanged(ref _slowMovePercentage, value);

                if (FastMovePercentage < value)
                {
                    FastMovePercentage = value;
                }

            }
        }
        public bool ControlsEnabled => _isEnabled.Value;
        public string ControlToggle => _controlToggle.Value;

        public bool IsDirectionNormal
        {
            get => _isDirectionNormal;
            set => this.RaiseAndSetIfChanged(ref _isDirectionNormal, value);
        }

        public bool IsDirectionReversed
        {
            get => _isDirectionReversed.Value;
            set => IsDirectionNormal = !value;
        }


        public StepperVM()
        {
            _encoderPosition = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                        ? Observable.Return("0.000")
                        : x.WhenAnyValue(y => y.EncoderPosition)
                           .Select(StringifyEncoder);
                })
                .Switch()
                .ToProperty(this, x => x.EncoderPosition);

            _encoderToolTip = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                        ? Observable.Return(new string[] { "0.000", "0.000", "" })
                        : Observable.CombineLatest(
                            x.WhenAnyValue(y => y.EncoderPosition)
                             .Select(StringifyEncoder),
                            x.WhenAnyValue(y => y.RawEncoderPosition)
                             .Select(y => StringifyEncoder((int)y)),
                            Observable.Return(""));

                })
                .Switch()
                .Select(arr => $"Encoder (um): {arr.ElementAt(0)}\nRawEncoder: {arr.ElementAt(1)}\nError {arr.ElementAt(2)}:")
                .ToProperty(this, x => x.EncoderToolTip);

            _stepperPosition = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                        ? Observable.Return("0.000")
                        : x.WhenAnyValue(y => y.StepperPosition)
                           .Select(StringifyStepper);
                })
                .Switch()
                .ToProperty(this, x => x.StepperPosition);

            _movingCCW = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                         ? Observable.Return(false)
                         : x.WhenAnyValue(y => y.MovingCCW);
                })
                .Switch()
                .ToProperty(this, x => x.MovingCCW);
            _movingCW = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                         ? Observable.Return(false)
                         : x.WhenAnyValue(y => y.MovingCW);
                })
                .Switch()
                .ToProperty(this, x => x.MovingCW);

            _hardLimitCCW = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                         ? Observable.Return(false)
                         : x.WhenAnyValue(y => y.HardLimitCCW);
                })
                .Switch()
                .ToProperty(this, x => x.HardLimitCCW);
            _hardLimitCW = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                         ? Observable.Return(false)
                         : x.WhenAnyValue(y => y.HardLimitCW);
                })
                .Switch()
                .ToProperty(this, x => x.HardLimitCW);

            _softLimitCCW = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                         ? Observable.Return(false)
                         : x.WhenAnyValue(y => y.SoftLimitCCW);
                })
                .Switch()
                .ToProperty(this, x => x.SoftLimitCCW);
            _softLimitCW = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                         ? Observable.Return(false)
                         : x.WhenAnyValue(y => y.SoftLimitCW);
                })
                .Switch()
                .ToProperty(this, x => x.SoftLimitCW);

            _notHomed = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                         ? Observable.Return(false)
                         : x.WhenAnyValue(y => y.NotHomed);
                })
                .Switch()
                .ToProperty(this, x => x.NotHomed);
            _homing = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                         ? Observable.Return(false)
                         : x.WhenAnyValue(y => y.Homing);
                })
                .Switch()
                .ToProperty(this, x => x.Homing);
            _homed = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                         ? Observable.Return(false)
                         : x.WhenAnyValue(y => y.Homed);
                })
                .Switch()
                .ToProperty(this, x => x.Homed);

            _isDeviceConnected = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                         ? Observable.Return(false)
                         : x.WhenAnyValue(y => y.IsDeviceConnected);
                })
                .Switch()
                .ToProperty(this, x => x.IsDeviceConnected);

            _isEnabled = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                         ? Observable.Return(false)
                         : x.WhenAnyValue(y => y.IsEnabled);
                })
                .Switch()
                .ToProperty(this, x => x.ControlsEnabled);

            _controlToggle = this
                .WhenAnyValue(x => x.Driver.IsEnabled)
                .Select(x => x ? "Disable" : "Enable")
                .ToProperty(this, x => x.ControlToggle);

            _isDirectionReversed = this
                .WhenAnyValue(x => x.IsDirectionNormal)
                .Select(x => !x)
                .ToProperty(this, x => x.IsDirectionReversed);

            _currentSavedPosition = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                         ? Observable.Return<IStepperDriver.ISavedPosition>(null)
                         : x.WhenAnyValue(y => y.CurrentSavedPosition);
                })
                .Switch()
                .ToProperty(this, x => x.CurrentSavedPosition);

            _savedPositions = this
                .WhenAnyValue(x => x.Driver)
                .Select(x =>
                {
                    return x is null
                         ? Observable.Return(Enumerable.Empty<IStepperDriver.ISavedPosition>())
                         : x.WhenAnyValue(y => y.SavedPositions);
                })
                .Switch()
                .ToProperty(this, x => x.SavedPositions);


            // Forward saved position information to the editor.
            this.WhenAnyValue(x => x.Driver)
                .Subscribe(x => SavedPositionsEditor.Driver = x);
        }




        /**
         * Translates the integer encoder value into a double value in um.
         */
        private string StringifyEncoder(int encoder_val)
        {
            double encoder = encoder_val * _driver.NmPerEncoderCount / 1e3;
            return encoder.ToString("0,0.000");
        }
        /**
         * Translates the integer stepper value into a double value in um.
         */
        private string StringifyStepper(int stepper_val)
        {
            double NM_PER_STEP = _driver.NmPerEncoderCount / _driver.StepperCountsPerEncoderCount;
            double pos = stepper_val * NM_PER_STEP / 1e3;
            return pos.ToString("0,0.000");
        }

        private readonly ObservableAsPropertyHelper<string> _encoderPosition;
        private readonly ObservableAsPropertyHelper<string> _encoderToolTip;
        private readonly ObservableAsPropertyHelper<string> _stepperPosition;
        private readonly ObservableAsPropertyHelper<bool> _movingCCW;
        private readonly ObservableAsPropertyHelper<bool> _movingCW;
        private readonly ObservableAsPropertyHelper<bool> _hardLimitCCW;
        private readonly ObservableAsPropertyHelper<bool> _hardLimitCW;
        private readonly ObservableAsPropertyHelper<bool> _softLimitCCW;
        private readonly ObservableAsPropertyHelper<bool> _softLimitCW;
        private readonly ObservableAsPropertyHelper<bool> _notHomed;
        private readonly ObservableAsPropertyHelper<bool> _homed;
        private readonly ObservableAsPropertyHelper<bool> _homing;
        private readonly ObservableAsPropertyHelper<bool> _isDeviceConnected;
        private readonly ObservableAsPropertyHelper<bool> _isEnabled;
        private readonly ObservableAsPropertyHelper<string> _controlToggle;
        private readonly ObservableAsPropertyHelper<bool> _isDirectionReversed;
        private readonly ObservableAsPropertyHelper<IStepperDriver.ISavedPosition> _currentSavedPosition;
        private readonly ObservableAsPropertyHelper<IEnumerable<IStepperDriver.ISavedPosition>> _savedPositions;

        private byte _fastMovePercentage = 100;
        private byte _slowMovePercentage = 100;
        private bool _isDirectionNormal = true;

        private IStepperDriver _driver = null;
    }
}
