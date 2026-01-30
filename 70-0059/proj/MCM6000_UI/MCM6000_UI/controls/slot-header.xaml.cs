using System;
using System.Collections.Generic;
using System.ComponentModel;
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

namespace MCM6000_UI.controls
{
    public class USBCallback
    {
        public delegate void WriteUSBDelegate(object general_param, byte[] bytes_to_write, int len);

        protected readonly object _class_param;
        private readonly WriteUSBDelegate _write_usb;

        public void WriteUSB(byte[] bytes_to_send, int len) => _write_usb(_class_param, bytes_to_send, len);

        public USBCallback(object class_param, WriteUSBDelegate write_usb)
        {
            _class_param = class_param;
            _write_usb = write_usb;
        }
    }


    public partial class slot_header : UserControl, INotifyPropertyChanged
    {
        // BEGIN    Private Fields
        private string _user_title;             // User Title String
        private string _stage_name;
        private byte _slot;
        private readonly OpenErrorHandler _error_handler;
        private uint _last_status_bits = 1;

        private USBCallback _callbacks;
        // END      Private Fields

        // BEGIN    Bound Properties
        public int Slot => _slot;
        public string SlotText { get; }
        public string ControlType { get; }
        public string UserTitle {
            get { return _user_title; }
            set
            {
                _user_title = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(
                    nameof(UserTitle)
                ));
            }
        }
        public string StageName { get { return _stage_name; } }
        public bool IsOkay { get => _last_status_bits == 0; }
        public Visibility ShowErrorHandler
        {
            get { return (_last_status_bits > 0) ? Visibility.Visible : Visibility.Collapsed; }
        }
        // END      Bound Properties

        // BEGIN    Events
        public class TitleChangedArgs
        {
            public byte Slot;
            public string Title;
        }
        public class GotStageNameArgs
        {
            public byte Slot;
            public string Name;
        }

        public delegate void TitleChangedHandler(object sender, TitleChangedArgs args);
        public delegate void GotStageNameHandler(object sender, GotStageNameArgs args);
        public delegate void OpenErrorHandler(byte slot, uint status_bits);

        public event PropertyChangedEventHandler? PropertyChanged;
        public event TitleChangedHandler? TitleChanged; // Sent when the controller's title changed.
        public event GotStageNameHandler? GotStageName; // Sent when the controller provided the stage name.

        public void InvokeTitleChanged(object obj, TitleChangedArgs args) { TitleChanged?.Invoke(obj, args); }
        public void InvokeGotStageName(object obj, GotStageNameArgs args) { GotStageName?.Invoke(obj, args); }
        // END      Events


        // BEGIN    Constructors
        public slot_header(byte slot_id, string control_type, USBCallback callback, OpenErrorHandler open_error)
        {
            _slot = slot_id;
            SlotText = "Slot " + (slot_id + 1).ToString();
            ControlType = control_type;
            _last_status_bits = 1;
            _error_handler = open_error;

            UserTitle = "";
            _stage_name = "Querying...";

            _callbacks = callback;

            TitleChanged += OnTitleChanged;
            GotStageName += OnGotStageName;

            InitializeComponent();

            DataContext = this;
        }
        // END      Constructors

        // BEGIN    Public Interface

        public void RequestSlotTitle()
        {
            byte[] bytes_to_send = new byte[6] { 0, 0, _slot, 0, Thorlabs.APT.Address.MOTHERBOARD, Thorlabs.APT.Address.HOST };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_REQ_SLOT_TITLE), 0, bytes_to_send, 0, 2);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        public void RequestPartNumber()
        {
            byte[] bytes_to_send = new byte[6] { 0, 0, _slot, 0, Thorlabs.APT.Address.MOTHERBOARD, Thorlabs.APT.Address.HOST };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_REQ_DEVICE), 0, bytes_to_send, 0, 2);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        public void UpdateStatusBits(byte slot, uint status_bits)
        {
            if (slot == _slot)
            {
                var last = _last_status_bits;
                _last_status_bits = status_bits;
                if (status_bits != last)
                {
                    PropertyChanged?.Invoke(this, new(nameof(ShowErrorHandler)));
                    PropertyChanged?.Invoke(this, new(nameof(IsOkay)));
                }
            }
        }

        public void Identify()
        {
            byte[] bytesToSend = new byte[6] { 0, 0, _slot, 0, Thorlabs.APT.Address.MOTHERBOARD, Thorlabs.APT.Address.HOST };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOD_IDENTIFY), 0, bytesToSend, 0, 2);
            
            _callbacks.WriteUSB(bytesToSend, bytesToSend.Length);
        }

        // END      Public Interface

        // BEGIN    Private Interface

        private void SetUserTitle(string new_title)
        {
            byte[] str = Encoding.ASCII.GetBytes(new_title);
            int bytes_to_copy = str.Length > 16 ? 16 : str.Length;

            byte[] bytes_to_send = new byte[24] {
                0, 0, 18, 0, (byte)(Thorlabs.APT.Address.MOTHERBOARD | 0x80), Thorlabs.APT.Address.HOST,
                _slot, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_SET_SLOT_TITLE), 0, bytes_to_send, 0, 2);
            Array.Copy(str, 0, bytes_to_send, 8, bytes_to_copy);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        private void OnTitleChanged(object sender, TitleChangedArgs args)
        {
            if (args.Slot == _slot)
            {
                UserTitle = args.Title;
            }
        }

        private void OnGotStageName(object sender, GotStageNameArgs args)
        {
            if (args.Slot == _slot)
            {
                _stage_name = args.Name;
                PropertyChanged?.Invoke(this, new(nameof(StageName)));
            }
        }

        // END      Private Interface

        // BEGIN    UI Event Bindings

        private void Button_SetSlotTitle_Clicked (object sender, RoutedEventArgs args)
        {
            string title = UserTitle;
            // TODO filter title down.
            SetUserTitle(title);
        }

        private void Button_OpenErrorHandler_Clicked (object? sender, RoutedEventArgs args)
        {
            _error_handler(_slot, _last_status_bits);
        }

        private void Button_Identify_Clicked(object sender, RoutedEventArgs args) => Identify();

        // END      UI Event Bindings
    }
}
