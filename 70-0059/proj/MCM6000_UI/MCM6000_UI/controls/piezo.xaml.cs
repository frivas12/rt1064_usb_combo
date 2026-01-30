using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
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
    /// <summary>
    /// Interaction logic for piezo.xaml
    /// </summary>
    public partial class piezo : Page
    {
        public class Callbacks : USBCallback
        {
            public delegate float GetNMPerCountDelegate(object general_param, byte slot_id);
            public delegate byte GetControlModeDelegate(object general_param, byte slot_id);

            private readonly GetNMPerCountDelegate _nm_per_count;
            private readonly GetControlModeDelegate _ctrl_mode;

            public float GetNMPerCount(byte slot_id) => _nm_per_count(_class_param, slot_id);
            public byte GetControlMode(byte slot_id) => _ctrl_mode(_class_param, slot_id);

            public Callbacks(object class_param, WriteUSBDelegate writeUSB, GetNMPerCountDelegate nm_per_count, GetControlModeDelegate ctrl_delegate) :
                base(class_param, writeUSB)
            {
                _nm_per_count = nm_per_count;
                _ctrl_mode = ctrl_delegate;
            }
        }


        const string PIEZO_TYPE_STRING = "Piezo";

        private Callbacks _callbacks;
        private byte _slot_id;
        private float _jog_size;

        // BEGIN    UI Properties
        public string JogSize
        {
            get { return _jog_size.ToString(); }
            set
            {
                try
                {
                    _jog_size = float.Parse(value);
                }
                catch { }
            }
        }
        // END      UI Properties

        // BEGIN    Constructors
        public piezo(byte slot_id, Callbacks cbks)
        {
            // Setting ui content.
            e_frm_header.Content = new slot_header(slot_id, PIEZO_TYPE_STRING, cbks, );

            // Setting private variables.
            _callbacks = cbks;
            _slot_id = slot_id;
            _jog_size = 10;

            // Initalizing UI.
            InitializeComponent();
            DataContext = this;
        }
        // END      Constructors

        // BEGIN    UI Event Handlers
        private void StopButtonClick(object sender, RoutedEventArgs args) => SetMode(Thorlabs.Piezo.Modes.PZ_STOP_MODE);

        private void AnalogInputModeClick(object sender, RoutedEventArgs args) => SetMode(Thorlabs.Piezo.Modes.PZ_ANALOG_INPUT_MODE);

        private void JogDownClick(object sender, RoutedEventArgs args) => MovePiezo(false);

        private void JogUpClick(object sender, RoutedEventArgs args) => MovePiezo(true);

        // END      UI Event Handlers

        private static bool CheckMoveBy(float percent)
        {
            bool rt = percent >= 0 && percent <= 100;
            if (!rt)
            {
                MessageBox.Show("Jogging value must be between 0 and 100.", "Error");
            }
            return rt;
        }

        private void MovePiezo(bool move_up)
        {
            float jog_size = _jog_size;
            float nm_per_count = _callbacks.GetNMPerCount(_slot_id);
            byte control_mode = _callbacks.GetControlMode(_slot_id);
            if (CheckMoveBy(jog_size))
            {
                int jv = (int)(jog_size * 1000 / nm_per_count) * ((move_up) ? 1 : -1);
                byte[] bytes_to_send = new byte[12];
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_PIEZO_SET_MOVE_BY), 0, bytes_to_send, 0, 2);
                Array.Copy(BitConverter.GetBytes(6), 0, bytes_to_send, 0, 2);
                bytes_to_send[4] = (byte)((Thorlabs.APT.Address.SLOT_1 + _slot_id) | 0x80);
                bytes_to_send[5] = Thorlabs.APT.Address.HOST;
                bytes_to_send[6] = _slot_id;
                bytes_to_send[7] = control_mode;
                Array.Copy(BitConverter.GetBytes(jv), 0, bytes_to_send, 8, 4);

                _callbacks.WriteUSB(bytes_to_send, 12);
                Thread.Sleep(20);
            }
        }

        private void SetMode(Thorlabs.Piezo.Modes mode)
        {
            byte[] bytes_to_send = new byte[6]
            {
                0, 0, (byte)mode, 0, (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id), Thorlabs.APT.Address.HOST
            };

            _callbacks.WriteUSB(bytes_to_send, 6);
            Thread.Sleep(50);
        }
    }
}
