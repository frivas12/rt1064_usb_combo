using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.ComponentModel;

namespace MCM6000_UI.controls
{
    /// <summary>
    /// Object that displays a PnP error and suggests fixes.
    /// When constructed, it saves what was previously displayed until the control is closed.
    /// </summary>
    public partial class pnp_error_handler : UserControl, IDisposable, INotifyPropertyChanged
    {
        public class Callbacks
        {
            private readonly byte _slot;

            public delegate void SetSlotSerialNumberToConnectedDeviceHandler(byte slot);
            public delegate void AllowCurrentlyConnectedDeviceTypeHandler(byte slot);

            private readonly SetSlotSerialNumberToConnectedDeviceHandler _set_callback;
            private readonly AllowCurrentlyConnectedDeviceTypeHandler _allow_callback;

            public void SetSlotSerialNumberToConnectedDevice() => _set_callback(_slot);
            public void AllowCurrentlyConnectedDeviceType() => _allow_callback(_slot);

            public Callbacks(byte slot, SetSlotSerialNumberToConnectedDeviceHandler setSlotSerialNumberToConnectedDevice,
                AllowCurrentlyConnectedDeviceTypeHandler allowCurrentlyConnectedDeviceType)
            {
                _slot = slot;
                _set_callback = setSlotSerialNumberToConnectedDevice;
                _allow_callback = allowCurrentlyConnectedDeviceType;
            }
        }


        // Flags
        const uint FLAG_NO_DEVICE_CONNECTED = 1 << 0;
        const uint FLAG_GENERAL_OW_ERROR = 1 << 1;
        const uint FLAG_UNKNOWN_OW_VERSION = 1 << 2;
        const uint FLAG_OW_CORRUPTION = 1 << 3;
        const uint FLAG_SERIAL_NUM_MISMATCH = 1 << 4;
        const uint FLAG_SIGNATURE_NOT_ALLOWED = 1 << 5;
        const uint FLAG_GENERAL_CONFIG_ERROR = 1 << 6;
        const uint FLAG_CONFIGURATION_SET_MISS = 1 << 7;
        const uint FLAG_CONFIGURATION_STRUCT_MISS = 1 << 8;

        private bool _disposed = false;
        private readonly ContentControl _targeted_control;
        private readonly UserControl? _previous_content = null;
        private readonly Callbacks _callbacks;

        private delegate void PnPErrorHandlerDelegate();
        private PnPErrorHandlerDelegate _yes_callback;
        private PnPErrorHandlerDelegate _no_callback;

        public event PropertyChangedEventHandler PropertyChanged;

        // Text shown to help the user.
        private string _guide_text;
        private bool _enable_yes;
        private bool _enable_no;
        public string GuideText
        {
            get { return _guide_text; }
            private set
            {
                _guide_text = value;
                PropertyChanged?.Invoke(this, new(nameof(GuideText)));
            }
        }
        public bool EnableYes
        {
            get { return _enable_yes; }
            private set
            {
                _enable_yes = value;
                PropertyChanged?.Invoke(this, new(nameof(EnableYes)));
            }
        }
        public bool EnableNo
        {
            get { return _enable_no; }
            private set
            {
                _enable_no = value;
                PropertyChanged?.Invoke(this, new(nameof(EnableNo)));
            }
        }

        public string HexStatus
        { get; }

        private static bool TestBits(uint num, uint bits) => (num & bits) > 0;

        public pnp_error_handler(ContentControl ctrl, uint status_flags, Callbacks callbacks)
        {
            HexStatus = string.Format("0x{0:X8}", status_flags);
            _targeted_control = ctrl;
            _callbacks = callbacks;

            // Just in case these get called, bind them to throw an exception.
            _yes_callback = NotSetErrorHandler;
            _no_callback = NotSetErrorHandler;

            // Setting Guide Text
            if (TestBits(status_flags, FLAG_NO_DEVICE_CONNECTED))
            {
                GuideText = "No device was detected.  Is a device connected?";
                EnableYes = true;
                EnableNo = true;
                _yes_callback = NotConnected_Yes;
                _no_callback = ExitErrorHandler;
            }
            else if (TestBits(status_flags, FLAG_UNKNOWN_OW_VERSION))
            {
                GuideText = "The Plug-and-Play version on the device is not supported by the controller.  Please seek support.";
                EnableYes = false;
                EnableNo = false;
            }
            else if (TestBits(status_flags, FLAG_OW_CORRUPTION))
            {
                GuideText = "The device's Plug-and-Play information has been marked as invalid.  Please, unplug and plug in the device." +
                    " If this continues, please seek support";
                EnableYes = false;
                EnableNo = false;
            }
            else if (TestBits(status_flags, FLAG_SERIAL_NUM_MISMATCH))
            {
                GuideText = "The device connected is not the device previously connected to the slot.  Please, check that cables are connected " +
                    "correctly.  Notice: this will also occur when setting up a new device for the first time.  Would you like to allow this device" +
                    " to run on the current slot?";
                EnableYes = true;
                EnableNo = true;
                _yes_callback = SerialNumberMismatch_Yes;
                _no_callback = ExitErrorHandler;
            }
            else if (TestBits(status_flags, FLAG_SIGNATURE_NOT_ALLOWED))
            {
                GuideText = "The device connected is not allowed to run on this slot.  Is this the first time seeing this error on this slot?";
                EnableYes = true;
                EnableNo = true;
                _yes_callback = SignatureNotAllowed_Yes;
                _no_callback = SignatureNotAllowed_No;
            }
            else if (TestBits(status_flags, FLAG_CONFIGURATION_SET_MISS))
            {
                GuideText = "A configuration for the device could not be found.  Please, update your look-up tables or contact support.";
                EnableYes = false;
                EnableNo = false;
            }
            else if (TestBits(status_flags, FLAG_CONFIGURATION_STRUCT_MISS))
            {
                GuideText = "The configuration for the device had an error.  Please, update your look-up tables or contact support.";
                EnableYes = false;
                EnableNo = false;
            }
            else
            {
                GuideText = "This Plug-and-Play error has no guide.  Please seek support.";
                EnableYes = false;
                EnableNo = false;
            }

            InitializeComponent();

            DataContext = this;

            if (ctrl.Content is not null)
            {
                _previous_content = ctrl.Content as UserControl;
            }

            Dispatcher.Invoke(() =>
            {
                ctrl.Content = this;
            });
        }

        private void Button_yes_Clicked(object? sender, RoutedEventArgs args) => _yes_callback();
        private void Button_no_Clicked(object? sender, RoutedEventArgs args) => _no_callback();
        private void Button_cancel_Clicked(object? sender, RoutedEventArgs args) => ExitErrorHandler();

        // Independent error handlers.
        private void NotSetErrorHandler()
        {
            throw new NotImplementedException("The error handler has not been bound");
        }
        private void ExitErrorHandler() => Dispose();

        // Status EventHandlers
        private void NotConnected_Yes()
        {
            GuideText = "The device's one-wire chip may be malfunctioning.  Seek support.";
            EnableYes = false;
            EnableNo = false;
        }
        private void SerialNumberMismatch_Yes()
        {
            _callbacks.SetSlotSerialNumberToConnectedDevice();

            GuideText = "The currenty connected device's unique identifer has been set.  This error should now clear.";
            EnableYes = false;
            EnableNo = false;
        }
        private void SignatureNotAllowed_Yes()
        {
            GuideText = "The currently connected device may not be allowed to run on this slot.  Would you like to allow it?";
            EnableYes = true;
            EnableNo = true;
            _yes_callback = SignatureNotAllowed_Yes_Yes;
            _no_callback = ExitErrorHandler;
        }
        private void SignatureNotAllowed_Yes_Yes()
        {
            _callbacks.AllowCurrentlyConnectedDeviceType();

            GuideText = "This device was allowed to run on this slot.  Please, check that this error has cleared.";
            EnableYes = false;
            EnableNo = false;
        }
        private void SignatureNotAllowed_No()
        {
            GuideText = "The slot card installed does not support the connected device.  Please, use a different slot or contact support.";
            EnableYes = false;
            EnableNo = false;
        }

        // External Hooks

        protected virtual void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                if (disposing)
                {
                    if (_previous_content is not null)
                    {
                        Dispatcher.Invoke(() => {
                            _targeted_control.Content = _previous_content;
                        });
                    }
                }

                _disposed = true;
            }
        }

        public void Dispose()
        {
            // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }
    }
}
