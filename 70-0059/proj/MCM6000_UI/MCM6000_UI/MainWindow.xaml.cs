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
using System.IO.Ports;
using System.IO;
using System.Timers;
using Microsoft.Win32;
using System.Threading;
using System.Windows.Threading;
using System.ComponentModel;
using System.Text.RegularExpressions;
using System.Runtime.InteropServices;


using Abt.Controls.SciChart;
using Abt.Controls.SciChart.Model.DataSeries;
using Abt.Controls.SciChart.Utility;
using Abt.Controls.SciChart.Visuals;
using Abt.Controls.SciChart.Visuals.Annotations;
using System.Xml;
using System.Reflection;
using System.Diagnostics;
using System.Reactive.Linq;
using ReactiveUI;
using System.Security.Permissions;
using System.Collections.Concurrent;
using System.Linq.Expressions;

namespace MCM6000_UI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    ///
    public partial class MainWindow : Window
    {
        const bool ENABLE_SYSTEM_LOGGING = false;
        const bool ENABLE_OVERHEAT_LOGGING = false;
        bool tabs_removed = false;
        byte chan_index = 0;
        private bool _boot_finished = false;

        static string assemblyVersion = Assembly.GetExecutingAssembly().GetName().Version.ToString();
        // string assemblyVersion1 = Assembly.LoadFile("your assembly file").GetName().Version.ToString();
        static string fileVersion = FileVersionInfo.GetVersionInfo(Assembly.GetExecutingAssembly().Location).FileVersion;
        static string productVersion = FileVersionInfo.GetVersionInfo(Assembly.GetExecutingAssembly().Location).ProductVersion;

        // **** change REVISION in  AssemblyInfo.cs under "Properties" in the tree ******
        static string software_version = fileVersion;

        const byte HOST_ID = 0x01;
        const byte SLOT_ID_BASE = 0x21;
        byte MOTHERBOARD_ID = 0x11; // 0x11 => MCM 1, 0x12 => MCM2 ...
        static byte[] header;
        static byte[] ext_data;

        StackPanel[,] cp_subpanels = new StackPanel[8, 5];
        Label[,] cp_labels = new Label[8, 20];
        RadioButton[,] cp_radio = new RadioButton[8, 20];
        Button[,] cp_button = new Button[8, 30];
        TextBox[,] cp_textbox = new TextBox[8, 20];

        static OutboundSerialMessageScheduler _txScheduler;
        static System.Timers.Timer _timer1;
        static string address;

        TabItem current_main_tab;

        
        const byte SLOT1 = 0;
        const byte SLOT2 = 1;
        const byte SLOT3 = 2;
        const byte SLOT4 = 3;
        const byte SLOT5 = 4;
        const byte SLOT6 = 5;
        const byte SLOT7 = 6;
        const byte SLOT8 = 7;

        const byte NUMBER_OF_BOARD_SLOTS = 8;
        bool ftdi_flag = false;

        const byte SHUTTER_OPENED = 1;
        const byte SHUTTER_CLOSED = 2;

        int usb_bytes;
        bool response_received = false;
        XmlDataProvider device_id_provider = new XmlDataProvider();
        System.Windows.Data.Binding binding = new System.Windows.Data.Binding();
        #region APT
        const int MGMSG_SET_SER_TO_EEPROM = 0x00A0;
        const int MGMSG_GET_SER_STATUS = 0x00A1;

        const int MGMSG_HW_REQ_INFO = 0x0005;
        const int MGMSG_HW_GET_INFO = 0x0006;
        const int MGMSG_MOD_SET_CHANENABLESTATE = 0x0210;
        const int MGMSG_MOD_REQ_CHANENABLESTATE = 0x0211;
        const int MGMSG_MOD_GET_CHANENABLESTATE = 0x0212;
        const int MGMSG_MOT_SET_POSCOUNTER = 0x0410;
        const int MGMSG_MOT_REQ_POSCOUNTER = 0x0411;
        const int MGMSG_MOT_GET_POSCOUNTER = 0x0412;
        const int MGMSG_MOT_SET_ENCCOUNTER = 0x0409;
        const int MGMSG_MOT_REQ_ENCCOUNTER = 0x040A;
        const int MGMSG_MOT_GET_ENCCOUNTER = 0x040B;
        const int MGMSG_MOT_SET_JOGPARAMS = 0x0416;
        const int MGMSG_MOT_REQ_JOGPARAMS = 0x0417;
        const int MGMSG_MOT_GET_JOGPARAMS = 0x0418;
        const int MGMSG_MOT_SET_HOMEPARAMS = 0x0440;
        const int MGMSG_MOT_REQ_HOMEPARAMS = 0x0441;
        const int MGMSG_MOT_GET_HOMEPARAMS = 0x0442;
        const int MGMSG_MOT_SET_LIMSWITCHPARAMS = 0x0423;
        const int MGMSG_MOT_REQ_LIMSWITCHPARAMS = 0x0424;
        const int MGMSG_MOT_GET_LIMSWITCHPARAMS = 0x0425;
        const int MGMSG_MOT_SET_POWERPARAMS = 0x0426;
        const int MGMSG_MOT_REQ_POWERPARAMS = 0x0427;
        const int MGMSG_MOT_GET_POWERPARAMS = 0x0428;
        const int MGMSG_MOT_SET_GENMOVEPARAMS = 0x043A;
        const int MGMSG_MOT_REQ_GENMOVEPARAMS = 0x043B;
        const int MGMSG_MOT_GET_GENMOVEPARAMS = 0x043C;
        const int MGMSG_MOT_MOVE_HOME = 0x0443;
        const int MGMSG_MOT_MOVE_RELATIVE = 0x0448;
        const int MGMSG_MOT_MOVE_ABSOLUTE = 0x0453;
        const int MGMSG_MOT_MOVE_JOG = 0x046A;
        const int MGMSG_MOT_MOVE_VELOCITY = 0x0457;
        const int MGMSG_MOT_MOVE_STOP = 0x0465;
        const int MGMSG_MOT_REQ_STATUSUPDATE = 0x0480;
        const int MGMSG_MOT_GET_STATUSUPDATE = 0x0481;
        const int MGMSG_MOT_SET_DCPIDPARAMS = 0x04A0;
        const int MGMSG_MOT_REQ_DCPIDPARAMS = 0x04A1;
        const int MGMSG_MOT_GET_DCPIDPARAMS = 0x04A2;



        const int MGMSG_MOT_SET_BUTTONPARAMS = 0x04B6;
        const int MGMSG_MOT_REQ_BUTTONPARAMS = 0x04B7;
        const int MGMSG_MOT_GET_BUTTONPARAMS = 0x04B8;

        const int MGMSG_MOT_SET_EEPROMPARAMS = 0x04B9;
        const int MGMSG_MOT_REQ_EEPROMPARAMS = 0x04BA;
        const int MGMSG_MOT_GET_EEPROMPARAMS = 0x04BB;

        const int MGMSG_MOT_SET_SOL_CYCLEPARAMS = 0x04C3;
        const int MGMSG_MOT_REQ_SOL_CYCLEPARAMS = 0x04C4;
        const int MGMSG_MOT_GET_SOL_CYCLEPARAMS = 0x04C5;

        const int MGMSG_MOT_SET_SOL_STATE = 0x04CB;
        const int MGMSG_MOT_REQ_SOL_STATE = 0x04CC;
        const int MGMSG_MOT_GET_SOL_STATE = 0x04CD;


        const int MGMSG_MOT_SET_PMDJOYSTICKPARAMS = 0x04E6;
        const int MGMSG_MOT_REQ_PMDJOYSTICKPARAMS = 0x04E7;
        const int MGMSG_MOT_GET_PMDJOYSTICKPARAMS = 0x04E8;

        const int MGMSG_MOT_SET_PMDSTAGEAXISPARAMS = 0x04F0;
        const int MGMSG_MOT_REQ_PMDSTAGEAXISPARAMS = 0x04F1;
        const int MGMSG_MOT_GET_PMDSTAGEAXISPARAMS = 0x04F2;
        const int MGMSG_LA_SET_MAGNIFICATION = 0x0840;
        const int MGMSG_LA_REQ_MAGNIFICATION = 0x0841;
        const int MGMSG_LA_GET_MAGNIFICATION = 0x0842;
        const int MGMSG_HS_GET_STATUSUPDATE = 0x0483;

        const int MGMSG_MOT_SET_MFF_OPERPARAMS = 0x0510;
        const int MGMSG_MOT_REQ_MFF_OPERPARAMS = 0x0511;
        const int MGMSG_MOT_GET_MFF_OPERPARAMS = 0x0512;


        // Laser Control Messages
        const int MGMSG_LA_SET_PARAMS = 0x0800;
        const int MGMSG_LA_REQ_PARAMS = 0x0801;
        const int MGMSG_LA_GET_PARAMS = 0x0802;
        const int MGMSG_LA_ENABLEOUTPUT = 0x0811;
        const int MGMSG_LA_DISABLEOUTPUT = 0x0812;

        //Firmware update command
        const int MGMSG_GET_UPDATE_FIRMWARE = 0x00A6;
        const int MGMSG_RESET_FIRMWARE_LOADCOUNT = 0x00A7;
        const int MGMSG_BL_REQ_FIRMWAREVER = 0x002F;
        const int MGMSG_BL_SET_FLASHPAGE = 0x00A8;
        const int MGMSG_BL_GET_FIRMWAREVER = 0x0030;

        //*****************************************************************
        // APT
        // 0x4000 to 0x4fff range are reserved for Sterling VA
        //*****************************************************************
        // System        
        const int MGMSG_MCM_HW_REQ_INFO = 0x4000;
        const int MGMSG_MCM_HW_GET_INFO = 0x4001;
        const int MGMSG_CPLD_UPDATE = 0x4002;
        const int MGMSG_SET_HW_REV = 0x4003;
        const int MGMSG_SET_CARD_TYPE = 0x4004;
        const int MGMSG_SET_DEVICE = 0x4005;
        const int MGMSG_REQ_DEVICE = 0x4006;
        const int MGMSG_GET_DEVICE = 0x4007;
        const int MGMSG_SET_DEVICE_BOARD = 0x4008;
        const int MGMSG_REQ_DEVICE_BOARD = 0x4009;
        const int MGMSG_GET_DEVICE_BOARD = 0x400A;
        const int MGMSG_RESTART_PROCESSOR = 0x400B;
        const int MGMSG_ERASE_EEPROM = 0x400C;
        const int MGMSG_REQ_CPLD_WR = 0x400D;
        const int MGMSG_GET_CPLD_WR = 0x400E;
        const int MGMSG_TASK_CONTROL = 0x400F;
        const int MGMSG_BOARD_REQ_STATUSUPDATE = 0x4010;
        const int MGMSG_BOARD_GET_STATUSUPDATE = 0x4011;
        const int MGMSG_MOD_REQ_JOYSTICK_INFO = 0x4012;
        const int MGMSG_MOD_GET_JOYSTICK_INFO = 0x4013;
        const int MGMSG_MOD_SET_JOYSTICK_MAP_IN = 0x4014;
        const int MGMSG_MOD_REQ_JOYSTICK_MAP_IN = 0x4015;
        const int MGMSG_MOD_GET_JOYSTICKS_MAP_IN = 0x4016;
        const int MGMSG_MOD_SET_JOYSTICK_MAP_OUT = 0x4017;
        const int MGMSG_MOD_REQ_JOYSTICK_MAP_OUT = 0x4018;
        const int MGMSG_MOD_GET_JOYSTICKS_MAP_OUT = 0x4019;
        const int MGMSG_MOD_SET_SYSTEM_DIM = 0x401A;
        const int MGMSG_MOD_REQ_SYSTEM_DIM = 0x401B;
        const int MGMSG_MOD_GET_SYSTEM_DIM = 0x401C;
        const int MGMSG_SET_STORE_POSITION_DEADBAND = 0x401D;
        const int MGMSG_REQ_STORE_POSITION_DEADBAND = 0x401E;
        const int MGMSG_GET_STORE_POSITION_DEADBAND = 0x401F;
        const int MGMSG_MCM_ERASE_DEVICE_CONFIGURATION = 0X4020;
        const int MGMSG_SET_STORE_POSITION = 0x4021;
        const int MGMSG_REQ_STORE_POSITION = 0x4022;
        const int MGMSG_GET_STORE_POSITION = 0x4023;
        const int MGMSG_SET_GOTO_STORE_POSITION = 0x4024;
        const int MGMSG_MCM_START_LOG = 0x4025;
        const int MGMSG_MCM_POST_LOG = 0x4026;
        const int MGMSG_MCM_SET_ENABLE_LOG = 0x4027;
        const int MGMSG_MCM_REQ_ENABLE_LOG = 0x4028;
        const int MGMSG_MCM_GET_ENABLE_LOG = 0x4029;
        const int MGMSG_MCM_REQ_JOYSTICK_DATA = 0x402A; // Only for the shuttlExpress joystick
        const int MGMSG_MCM_GET_JOYSTICK_DATA = 0x402B;

        // Stepper
        const int MGMSG_MCM_SET_SOFT_LIMITS = 0x403D;
        const int MGMSG_MCM_SET_HOMEPARAMS = 0x403E;
        const int MGMSG_MCM_REQ_HOMEPARAMS = 0x403F;
        const int MGMSG_MCM_GET_HOMEPARAMS = 0x4040;
        const int MGMSG_MCM_SET_STAGEPARAMS = 0x4041;
        const int MGMSG_MCM_REQ_STAGEPARAMS = 0x4042;
        const int MGMSG_MCM_GET_STAGEPARAMS = 0x4043;
        const int MGMSG_MCM_REQ_STATUSUPDATE = 0x4044;
        const int MGMSG_MCM_GET_STATUSUPDATE = 0x4045;
        const int MGMSG_MCM_SET_ABS_LIMITS = 0x4046;
        const int MGMSG_MCM_MOT_SET_LIMSWITCHPARAMS = 0x4047;
        const int MGMSG_MCM_MOT_REQ_LIMSWITCHPARAMS = 0x4048;
        const int MGMSG_MCM_MOT_GET_LIMSWITCHPARAMS = 0x4049;
        const int MGMSG_MCM_MOT_MOVE_BY = 0x4050;	// Added for Texas TIDE autofocus
        const int MGMSG_MCM_REQ_STEPPER_LOG = 0x4051;
        const int MGMSG_MCM_GET_STEPPER_LOG = 0x4052;
        const int MGMSG_MCM_MOT_SET_VELOCITY = 0x4053;

        // Shutter
        const int MGMSG_MCM_SET_SHUTTERPARAMS = 0x4064;
        const int MGMSG_MCM_REQ_SHUTTERPARAMS = 0x4065;
        const int MGMSG_MCM_GET_SHUTTERPARAMS = 0x4066;
        const int MGMSG_MCM_REQ_INTERLOCK_STATE = 0x4067;
        const int MGMSG_MCM_GET_INTERLOCK_STATE = 0x4068;

        // Slider IO
        const int MGMSG_MCM_SET_MIRROR_STATE = 0x4087;
        const int MGMSG_MCM_REQ_MIRROR_STATE = 0x4088;
        const int MGMSG_MCM_GET_MIRROR_STATE = 0x4089;
        const int MGMSG_MCM_SET_MIRROR_PARAMS = 0x408A;
        const int MGMSG_MCM_REQ_MIRROR_PARAMS = 0x408B;
        const int MGMSG_MCM_GET_MIRROR_PARAMS = 0x408C;
        const int MGMSG_MCM_SET_MIRROR_PWM_DUTY = 0x408D;
        const int MGMSG_MCM_REQ_MIRROR_PWM_DUTY = 0x408E;
        const int MGMSG_MCM_GET_MIRROR_PWM_DUTY = 0x408F;

        // Synchronized Motion   
        const int MGMSG_MCM_SET_HEX_POSE = 0x40A0;
        const int MGMSG_MCM_REQ_HEX_POSE = 0x40A1;
        const int MGMSG_MCM_GET_HEX_POSE = 0x40A2;
        const int MGMSG_MCM_SET_SYNC_MOTION_PARAM = 0x40A3;
        const int MGMSG_MCM_REQ_SYNC_MOTION_PARAM = 0x40A4;
        const int MGMSG_MCM_GET_SYNC_MOTION_PARAM = 0x40A5;
        const int MGMSG_MCM_SET_SYNC_MOTION_POINT = 0x40A6;

        // OTM Laser
        const int MGMSG_LA_DISABLEAIMING = 0x40C7;
        const int MGMSG_LA_ENABLEAIMING = 0x40C8;

        // Piezo
        const int MGMSG_MCM_PIEZO_SET_PRAMS = 0x40DC;
        const int MGMSG_MCM_PIEZO_REQ_PRAMS = 0x40DD;
        const int MGMSG_MCM_PIEZO_GET_PRAMS = 0x40DE;
        const int MGMSG_MCM_PIEZO_REQ_VALUES = 0x40DF;
        const int MGMSG_MCM_PIEZO_GET_VALUES = 0x40E0;
      //  const int MGMSG_MCM_PIEZO_REQ_LOG = 0x40E1;
        const int MGMSG_MCM_PIEZO_GET_LOG = 0x40E2;
        const int MGMSG_MCM_PIEZO_SET_ENABLE_PLOT = 0x40E3;
        const int MGMSG_MCM_PIEZO_SET_DAC_VOLTS = 0x40E4;
        const int MGMSG_MCM_PIEZO_SET_PID_PARMS = 0x40E5;
        const int MGMSG_MCM_PIEZO_REQ_PID_PARMS = 0x40E6;
        const int MGMSG_MCM_PIEZO_GET_PID_PARMS = 0x40E7;
        const int MGMSG_MCM_PIEZO_SET_MOVE_BY = 0x40E8;
        const int MGMSG_MCM_PIEZO_SET_MODE = 0x40E9;
        const int MGMSG_MCM_PIEZO_REQ_MODE = 0x40EA;
        const int MGMSG_MCM_PIEZO_GET_MODE = 0x40EB;

        // One-Wire
        const int MGMSG_OW_SET_PROGRAMMING = 0x40EC;
        const int MGMSG_OW_REQ_PROGRAMMING = 0x40ED;
        const int MGMSG_OW_GET_PROGRAMMING = 0x40EE;
        const int MGMSG_OW_PROGRAM = 0x40EF;
        const int MGMSG_OW_REQ_PROGRAMMING_SIZE = 0x40F0;
        const int MGMSG_OW_GET_PROGRAMMING_SIZE = 0x40F1;

        const int MGMSG_MCM_SET_ALLOWED_DEVICES = 0x40F2;
        const int MGMSG_MCM_REQ_ALLOWED_DEVICES = 0x40F3;
        const int MGMSG_MCM_GET_ALLOWED_DEVICES = 0x40F4;

        const int MGMSG_MCM_SET_DEVICE_DETECTION = 0x40F5;
        const int MGMSG_MCM_REQ_DEVICE_DETECTION = 0x40F6;
        const int MGMSG_MCM_GET_DEVICE_DETECTION = 0x40F7;

        // Look Up Table
        const int MGMSG_LUT_SET_INQUIRY = 0x40F8;
        const int MGMSG_LUT_RES_INQUIRY = 0x40F9;
        const int MGMSG_LUT_REQ_INQUIRY = 0x40FA;
        const int MGMSG_LUT_GET_INQUIRY = 0x40FB;
        const int MGMSG_LUT_INQUIRE = 0x40FC;
        const int MGMSG_LUT_INQUIRE_RES = 0x40FD;
        const int MGMSG_LUT_SET_PROGRAMMING = 0x40FE;
        const int MGMSG_LUT_RES_PROGRAMMING = 0x40FF;
        const int MGMSG_LUT_REQ_PROGRAMMING = 0x4100;
        const int MGMSG_LUT_GET_PROGRAMMING = 0x4101;
        const int MGMSG_LUT_PROGRAM = 0x4102;
        const int MGMSG_LUT_PROGRAM_RES = 0x4103;
        const int MGMSG_LUT_FINISH_PROGRAMMING = 0x4104;
        const int MGMSG_LUT_FINISH_PROGRAMMING_RES = 0x4105;
        const int MGMSG_LUT_REQ_PROGRAMMING_SIZE = 0x4106;
        const int MGMSG_LUT_GET_PROGRAMMING_SIZE = 0x4107;


        //*****************************************************************
        const string THORLABS_VID = "1313";
        const string THORLABS_MCM6000_PID = "2003";
        const string THORLABS_MCM301_PID = "2016";
        const string THORLABS_MCM6101_PID = "201E";
        readonly string[] THORLABS_ALLOWED_PIDS = new string[] { THORLABS_MCM6000_PID, THORLABS_MCM301_PID, THORLABS_MCM6101_PID };

        const string FTDI_VID = "0403";
        const string FTDI_PID = "6015";

        const string THORLABS_MCM_BOOTLOADER_PID = "2F03";

        // firmware loader
        static bool in_bootloader;
        static bool loading_firmware;
        static bool loading_cpld;
        const int S70_PAGE_SIZE = 512;
        static AutoResetEvent firmware_transfer_done = new AutoResetEvent(false);
        // cpld programming
        static byte cpld_data_recieved_flag = 0;
        static byte cpld_data_recieved = 0;

        #endregion

        public bool IsConnected { get; set; }

        public MainWindow()
        {
            // (sbenish): This needs to happen before component initialization.
            _supportedOnewireDevices = new(GetSupportedDevices, LazyThreadSafetyMode.PublicationOnly);

            InitializeComponent();

            _efs_driver = new((d, o, l) =>
            {
                byte[] data = d;
                if (o != 0)
                {
                    data = new byte[data.Length - o];
                    Array.Copy(d, o, data, 0, data.Length);
                }
                usb_write("efs driver", data, l);
            });
            EventHandler f = (a, B) =>
            {
                if (_efs_updating_ui)
                {
                    efs_refresh_ui();
                }
            };
            _efs_driver.FilesChanged += f;
            _efs_driver.HeaderChanged += f;

            in_bootloader = false;
            loading_firmware = false;
            loading_cpld = false;
#if false
            if (HasTouchInput())
            {
                this.WindowState = WindowState.Maximized;
                this.WindowStyle = WindowStyle.None;
                this.Topmost = true;
            }
#endif
            lbl_con_status.Dispatcher.Invoke(new Action(() =>
            { lbl_con_status.Content = "Disconnected"; }));

            // system_tab_init();

            this.Closed += MainWindow_Closed;
            this.Loaded += MainWindow_Loaded;

            initialize_data_series_plots();
            initialize_piezo_plots();
        }

        private void remove_tabs()
        {
            try
            {
                if (ENABLE_SYSTEM_LOGGING)
                {
                    MCM6000_tabs.Items.Remove(Log_tab);
                }
                ControlTab.Visibility = Visibility.Collapsed;
                hexapod_tab.Visibility = Visibility.Collapsed;
                vritual_motion.Visibility = Visibility.Collapsed;
                stepper_log_tab.Visibility = Visibility.Collapsed;
                USB_host.Visibility = Visibility.Collapsed;
                Steppers.Visibility = Visibility.Collapsed;
                Servo.Visibility = Visibility.Collapsed;
                Shutter.Visibility = Visibility.Collapsed;
                IO.Visibility = Visibility.Collapsed;
                Shutter_4.Visibility = Visibility.Collapsed;
                Elliptec.Visibility = Visibility.Collapsed;
                Piezo_tab.Visibility = Visibility.Collapsed;
                system_tab.Visibility = Visibility.Collapsed;
                efs_tab.Visibility = Visibility.Collapsed;
                ow_tab.Visibility = Visibility.Collapsed;
                tabs_removed = true;
        }
            catch (Exception)
            {

            }
}

        private void add_tabs()
        {
            try
            {

                if (ENABLE_SYSTEM_LOGGING)
                {
                    MCM6000_tabs.Items.Add(Log_tab);
                }
                ControlTab.Visibility = Visibility.Visible;
                MesoTab.Visibility = Visibility.Visible;
                hexapod_tab.Visibility = Visibility.Visible;
                vritual_motion.Visibility = Visibility.Visible;
                stepper_log_tab.Visibility = Visibility.Visible;
                USB_host.Visibility = Visibility.Visible;
                Steppers.Visibility = Visibility.Visible;
                Servo.Visibility = Visibility.Visible;
                Shutter.Visibility = Visibility.Visible;
                IO.Visibility = Visibility.Visible;
                Shutter_4.Visibility = Visibility.Visible;
                Elliptec.Visibility = Visibility.Visible;
                Piezo_tab.Visibility = Visibility.Visible;
                system_tab.Visibility = Visibility.Visible;
                tabs_removed = false;
            }
            catch (Exception)
            {

            }
        }
        private void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            if (ENABLE_SYSTEM_LOGGING)
            {
                // load the log file to the log tab textbox
                load_log_file();
                load_log_xml_file();
            }
            else
            {
            // hide the log tab
                MCM6000_tabs.Items.Remove(Log_tab);
                cb_enable_logging.Visibility = Visibility.Hidden;
            }
            // Get the file's location.
            string filename = System.AppDomain.CurrentDomain.BaseDirectory;
            filename = System.IO.Path.GetFullPath(filename) + "xml\\system\\devices\\device_id.xml";

            XmlDataProvider provider = (XmlDataProvider)FindResource("Device_id_XmlData");
            provider.Source = new Uri(filename);
            provider.Refresh();

            IsConnected = false;
            _txScheduler = new(new SerialPort());
            _usbWriterThread = new Thread(new ThreadStart(UsbWriterThread));
            _usbWriterThread.Start();


            current_main_tab = MCM6000_tabs.SelectedValue as TabItem;

            _timer1 = new System.Timers.Timer(50);
            _timer1.Elapsed += _timer1_Elapsed;
            _timer1.AutoReset = true;
            _timer1.Start();
            tbx_password.Visibility = Visibility.Collapsed;
            e_btn_boardIdentify.Visibility = Visibility.Collapsed;

            // if the config.txt file exisit then all the tabs will show
            // Set the tab you want opened when UI starts
            if(File.Exists("config.txt")) 
            {
                MCM6000_tabs.SelectedItem = ControlTab; // ControlTab; // Steppers, USB_host system_tab;
                if (tabs_removed)
                    add_tabs();
            }
            else // show only meso tab
            {
                Application.Current.MainWindow.SizeToContent = SizeToContent.Manual;
                Application.Current.MainWindow.WindowState = WindowState.Maximized;
                MCM6000_tabs.SelectedItem = MesoTab; // ControlTab; // Steppers, USB_host system_tab;
                remove_tabs();
            }

            ow_init();

#if true    // Temp hide move buttos on meso tab (*** these have problem on tablet ***)
            btnMeso_CCW_F_1.Visibility = Visibility.Hidden;
            btnMeso_CCW_F_2.Visibility = Visibility.Hidden;
            btnMeso_CCW_F_3.Visibility = Visibility.Hidden;
            btnMeso_CCW_F_4.Visibility = Visibility.Hidden;
            btnMeso_CCW_S_1.Visibility = Visibility.Hidden;
            btnMeso_CCW_S_2.Visibility = Visibility.Hidden;
            btnMeso_CCW_S_3.Visibility = Visibility.Hidden;
            btnMeso_CCW_S_4.Visibility = Visibility.Hidden;
            btnMeso_CW_S_1.Visibility = Visibility.Hidden;
            btnMeso_CW_S_2.Visibility = Visibility.Hidden;
            btnMeso_CW_S_3.Visibility = Visibility.Hidden;
            btnMeso_CW_S_4.Visibility = Visibility.Hidden;
            btnMeso_CW_F_1.Visibility = Visibility.Hidden;
            btnMeso_CW_F_2.Visibility = Visibility.Hidden;
            btnMeso_CW_F_3.Visibility = Visibility.Hidden;
            btnMeso_CW_F_4.Visibility = Visibility.Hidden;
#endif

            _boot_finished = true;
        }
        // COM port, VIDhex:PIDhex
        private Tuple<string, string> Try_connecting_to_PC_port(string pid)
        {
            int index = 0, deviceCount = 0;
            string hashName = string.Empty;
            string[] portNames;

            String device_pattern = String.Format("VID_{0}.*&.*PID_{1}", THORLABS_VID, pid);
            Regex _rx_pc = new Regex(device_pattern, RegexOptions.IgnoreCase);
            RegistryKey rk_pc = Registry.LocalMachine;
            RegistryKey pc_Enum;

            // Try to connect to the PC port
            try
            {
                pc_Enum = rk_pc.OpenSubKey("SYSTEM\\CurrentControlSet\\services\\usbser\\Enum");
                while (null != pc_Enum.GetValue((string)index.ToString(), null))
                {
                    string nameData = (string)pc_Enum.GetValue((string)index.ToString());
                    if (_rx_pc.Match(nameData).Success)
                    {
                        deviceCount++;
                        portNames = _rx_pc.Split(nameData);
                        // Save the string after PID_###&VID###, this is the hashValue of the bootloader device, use substring to ignore the '\'
                        if (1 < portNames.Length)
                        {
                            hashName = portNames[1].Substring(1).ToLower();
                            in_bootloader = false;
                        }
                    }
                    index++;
                }
            }
            catch (Exception)
            {
                return null;
            }

            if (1 < deviceCount)
            {
                //  MessageBox.Show("Too many devices are connected, make sure only the device you want to update is connected.");
                return null;
            }

            if (1 > deviceCount)
            {
                //  MessageBox.Show("Could not find any connected device", "Error");
                return null;
            }

            //Grab the HID generated by Windows as a hashvalue and use it to find the correct COM port
            RegistryKey rk2 = rk_pc.OpenSubKey("SYSTEM\\CurrentControlSet\\Enum");
            foreach (String s3 in rk2.GetSubKeyNames())
            {
                RegistryKey rk3 = rk2.OpenSubKey(s3);
                foreach (String s in rk3.GetSubKeyNames())
                {
                    // firmware
                    if ((_rx_pc.Match(s).Success))
                    {
                        RegistryKey rk4 = rk3.OpenSubKey(s);
                        foreach (String s2 in rk4.GetSubKeyNames())
                        {
                            if (0 == s2.ToLower().CompareTo(hashName))
                            {
                                RegistryKey rk5 = rk4.OpenSubKey(s2);
                                RegistryKey rk6 = rk5.OpenSubKey("Device Parameters");
                                var portName = (string)rk6.GetValue("PortName");
                                ftdi_flag = false;
                                return Tuple.Create(portName, $"{THORLABS_VID:X04}:{pid:X04}");
                            }
                        }
                    }
                }
            }

            return null;
        }

        // (COM??, VIDhex:PIDhex)
        private Tuple<string, string> Try_connecting_to_bootloader_port()
        {
            int index = 0, deviceCount = 0;
            string nameData = string.Empty;
            string portName = string.Empty;
            string hashName = string.Empty;
            string[] portNames;

            String device_pattern = String.Format("VID_{0}.*&.*PID_{1}", THORLABS_VID, THORLABS_MCM_BOOTLOADER_PID);
            Regex _rx_pc = new Regex(device_pattern, RegexOptions.IgnoreCase);
            RegistryKey rk_pc = Registry.LocalMachine;
            RegistryKey pc_Enum;

            // Try to connect to the PC port
            try
            {
                pc_Enum = rk_pc.OpenSubKey("SYSTEM\\CurrentControlSet\\services\\usbser\\Enum");
                while (null != pc_Enum.GetValue((string)index.ToString(), null))
                {
                    nameData = (string)pc_Enum.GetValue((string)index.ToString());
                    if (_rx_pc.Match(nameData).Success)
                    {
                        deviceCount++;
                        portNames = _rx_pc.Split(nameData);
                        // Save the string after PID_###&VID###, this is the hashValue of the bootloader device, use substring to ignore the '\'
                        if (1 < portNames.Length)
                        {
                            hashName = portNames[1].Substring(1);
                            in_bootloader = true;
                        }
                    }
                    index++;
                }
            }
            catch (Exception)
            {
                return null;
            }

            if (1 < deviceCount)
            {
                //  MessageBox.Show("Too many devices are connected, make sure only the device you want to update is connected.");
                return null;
            }

            if (1 > deviceCount)
            {
                //  MessageBox.Show("Could not find any connected device", "Error");
                return null;
            }

            //Grab the HID generated by Windows as a hashvalue and use it to find the correct COM port
            RegistryKey rk2 = rk_pc.OpenSubKey("SYSTEM\\CurrentControlSet\\Enum");
            foreach (String s3 in rk2.GetSubKeyNames())
            {
                RegistryKey rk3 = rk2.OpenSubKey(s3);
                foreach (String s in rk3.GetSubKeyNames())
                {
                    // firmware
                    if ((_rx_pc.Match(s).Success))
                    {
                        RegistryKey rk4 = rk3.OpenSubKey(s);
                        foreach (String s2 in rk4.GetSubKeyNames())
                        {
                            if (0 == s2.CompareTo(hashName))
                            {
                                RegistryKey rk5 = rk4.OpenSubKey(s2);
                                RegistryKey rk6 = rk5.OpenSubKey("Device Parameters");
                                portName = (string)rk6.GetValue("PortName");
                                ftdi_flag = false;
                                return Tuple.Create(portName, $"{THORLABS_VID:X04}:{THORLABS_MCM_BOOTLOADER_PID:X04}");
                            }
                        }
                    }
                }
            }

            return null;
        }

        // (COM??, VIDhex:PIDhex)
        private Tuple<string, string> Try_connecting_to_ftdi_port()
        {
            int index = 0;

            Tuple<string, string> ftdiVidPid = Tuple.Create(FTDI_VID, FTDI_PID);
            IEnumerable<string> thorlabsFtdiPortPids = ["201F"];
            Tuple<string, string>[] allowedFtdiPorts = thorlabsFtdiPortPids.Select(x => Tuple.Create(THORLABS_VID, x)).Append(ftdiVidPid).ToArray();

            RegistryKey rk_ftdi = Registry.LocalMachine;
            RegistryKey ftdi_Enum;

            // (vid, pid, hashString)
            List<Tuple<string, string, string>> matches = new();

            // Try to connect to the FTDI port
            try
            {
                ftdi_Enum = rk_ftdi.OpenSubKey("SYSTEM\\CurrentControlSet\\services\\FTDIBUS\\Enum");
                Regex[] regexes = allowedFtdiPorts.Select(x => new Regex($"VID_{x.Item1}.*PID_{x.Item2}", RegexOptions.IgnoreCase)).ToArray();
                while (null != ftdi_Enum.GetValue((string)index.ToString(), null))
                {
                    string nameData = (string)ftdi_Enum.GetValue((string)index.ToString());
                    for(int i = 0; i < regexes.Length; ++i)
                    {
                        var r = regexes[i];
                        if (r.Match(nameData).Success)
                        {
                            var split = r.Split(nameData);
                            if (1 <= split.Length)
                            {
                                // Save the string after PID_###&VID###, this is the hashValue of the bootloader device, use substring to ignore the '\'
                                var hashName = split[1].Substring(1);

                                matches.Add(Tuple.Create(allowedFtdiPorts[i].Item1, allowedFtdiPorts[i].Item2, hashName));
                            }
                        }
                    }
                    index++;
                }
            }
            catch (Exception)
            {
                return null;
            }

            var deviceCount = matches.Count;

            if (1 < deviceCount)
            {
                //  MessageBox.Show("Too many devices are connected, make sure only the device you want to update is connected.");
                return null;
            }

            if (1 > deviceCount)
            {
                //  MessageBox.Show("Could not find any connected device", "Error");
                return null;
            }

            var matchRegex = new Regex($"VID_{matches.First().Item1}.*PID_{matches.First().Item2}.*{matches.First().Item3}", RegexOptions.IgnoreCase);

            //Grab the HID generated by Windows as a hashvalue and use it to find the correct COM port
            RegistryKey rk2 = rk_ftdi.OpenSubKey("SYSTEM\\CurrentControlSet\\Enum");
            foreach (String s3 in rk2.GetSubKeyNames())
            {
                RegistryKey rk3 = rk2.OpenSubKey(s3);
                foreach (String s in rk3.GetSubKeyNames())
                {
                    if (matchRegex.Match(s).Success)
                    {
                        RegistryKey rk4 = rk3.OpenSubKey(s);

                        foreach (String s2 in rk4.GetSubKeyNames())
                        {
                            Console.WriteLine($"{s3}/{s}/{s2}");
                            RegistryKey rk5 = rk4.OpenSubKey(s2);
                            RegistryKey rk6 = rk5.OpenSubKey("Device Parameters");
                            if (null != rk6)
                            {
                                var portName = (string)rk6.GetValue("PortName");
                                ftdi_flag = true;
                                return Tuple.Create(portName, $"{matches.First().Item1:X04}:{matches.First().Item2:X04}");
                            }
                        }
                    }
                }
            }

            return null;
        }


        private void MainWindow_Closed(object sender, EventArgs e)
        {
            flush_log_buffer();
            cb_piezo_plot_disable();
            _usbWriteProfiler.Dispose();
            _usbWriterThreadExit.Set();
            _usbWriterThread.Join();
            /*
               IsConnected = false;
               _timer1.Enabled = false;
               _timer1.Stop();
               // _txScheduler.Port.Close();
               usb_disconnect();*/
        }

        private bool try_to_open_usb_port()
        {
            try
            {
                _txScheduler.Port.PortName = address;
                if (ftdi_flag)
                    _txScheduler.Port.BaudRate = 115200;// 460800 500000 921600;
                else
                    _txScheduler.Port.BaudRate = 115200;// 115200;// needs to be set for tablet
                _txScheduler.Port.Parity = System.IO.Ports.Parity.None;
                _txScheduler.Port.DataBits = 8;
                _txScheduler.Port.StopBits = System.IO.Ports.StopBits.One;

                _txScheduler.Port.Open();

                _txScheduler.Port.DiscardOutBuffer();
                _txScheduler.Port.DiscardInBuffer();
               // Thread.Sleep(500);
                return true;
            }
            catch (Exception)
            {
                Thread.Sleep(5);
                return false;
            }
        }

            private static controls.stepper.Callbacks.WriteUSBDelegate BindProfile(string profile) => (a, b, c) => usb_write(a, profile, b, c);
            private static void usb_write(object obj, string profile, byte[] bytes_to_write, int bytes_to_send)
            {
                MainWindow mv = obj as MainWindow;
                mv.usb_write(profile, bytes_to_write, bytes_to_send);
            }

            private void usb_connect()
            {
                Thread.Sleep(100);
                var tup = Try_connecting_to_bootloader_port();
                if (tup != null)
                {
                    (address, _programmingPath.EncryptionKey) = tup;
                    if (try_to_open_usb_port())
                    {
                        _txScheduler.CooldownMs = 15;
                        goto next;
                    }
                }

                foreach (string pid in THORLABS_ALLOWED_PIDS)
                {
                    tup = Try_connecting_to_PC_port(pid);
                    if (tup != null)
                    {
                        break;
                    }
                }
                if (tup != null)
                {
                    (address, _programmingPath.EncryptionKey) = tup;
                    if(try_to_open_usb_port())
                    {
                        _txScheduler.CooldownMs = 15;
                        goto next;
                    }
                }
                Thread.Sleep(100); 
                tup = Try_connecting_to_ftdi_port();
                if (tup != null)
                {
                    (address, _programmingPath.EncryptionKey) = tup;
                    if (try_to_open_usb_port())
                    {
                        _txScheduler.CooldownMs = 30;
                        goto next;
                    }
                }
                next:

                byte[] hwinfo = new byte[256];
                byte[] bytesToSend;
                try
                {
                    /*
                         _txScheduler.Port.PortName = address;
                         if (ftdi_flag)
                             _txScheduler.Port.BaudRate = 921600;// 460800 500000 921600;
                         else
                             _txScheduler.Port.BaudRate = 115200;// 115200;// needs to be set for tablet
                         _txScheduler.Port.Parity = System.IO.Ports.Parity.None;
                         _txScheduler.Port.DataBits = 8;
                         _txScheduler.Port.StopBits = System.IO.Ports.StopBits.One;

                         _txScheduler.Port.Open();

                         _txScheduler.Port.DiscardOutBuffer();
                         _txScheduler.Port.DiscardInBuffer();

                         Thread.Sleep(500); */

                    if (in_bootloader)
                    {
                        goto bootloader_skip;
                    }

                    Thread.Sleep(30);
                    byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_HW_REQ_INFO));
                    bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, MOTHERBOARD_ID, HOST_ID };

                send_again:
                    _txScheduler.SendAsap(bytesToSend, 0, 6);
                    int timeout = 0;
                    while (_txScheduler.Port.BytesToRead < 89)
                    {
                        if (timeout > 50)
                        {
                       //     _txScheduler.Port.DiscardOutBuffer();
                        //    _txScheduler.Port.DiscardInBuffer();
                            goto send_again;
                        }
                        timeout++;
                        Thread.Sleep(10);
                    };  //block waiting for whole response
                    _txScheduler.Port.Read(hwinfo, 0, _txScheduler.Port.BytesToRead);

                    hardware_info(hwinfo);

            bootloader_skip:
                //attach interrupt handler
                _txScheduler.Port.DataReceived += SerialPort_DataReceived;


                    lbl_con_status.Dispatcher.Invoke(new Action(() =>
                    {
                        if (!in_bootloader)
                        {
                            if (!ftdi_flag)
                                lbl_con_status.Content = "Connected to PC Port";
                            else
                                lbl_con_status.Content = "Connected to Tablet Port";
                            e_btn_boardIdentify.Visibility = Visibility.Visible;
                        }
                        else
                            lbl_con_status.Content = "Connected to Bootloader";

                        lbl_con_status.Foreground = Brushes.White;
                        MCM6000_tabs.IsEnabled = true;
                    }));
                  //  Thread.Sleep(50);

                    //setup and start timer
                    _timer1.Interval = 15;
                    IsConnected = true;

                    if (in_bootloader)
                    {
                        this.Dispatcher.Invoke(new Action(() =>
                        {
                            current_main_tab = system_tab;
                            system_tab.IsSelected = true;
                            btEnter_Exit_BootLoader.Content = "Enter Application";
                            btn_select_firmware_file.Visibility = Visibility.Visible;
                            btn_select_firmware_file.Visibility = Visibility.Visible;
                            labelFirmware_rev_val.Visibility = Visibility.Visible;
                        }));
                        try
                        {
                            byte[] command_3 = BitConverter.GetBytes(Convert.ToInt32(MGMSG_BL_REQ_FIRMWAREVER));
                            bytesToSend = new byte[6] { command_3[0], command_3[1], 0x00, 0x00, MOTHERBOARD_ID, HOST_ID };
                            usb_write("bl_firmware_ver", bytesToSend, 6);
                            Thread.Sleep(30);
                        }
                        catch (Exception)
                        { }
                        return;
                    }
                    else
                    {
                        this.Dispatcher.Invoke(new Action(() =>
                        {
                            btEnter_Exit_BootLoader.Content = "Enter Bootloader";
                            btn_select_firmware_file.Visibility = Visibility.Hidden;
                            lbl1pbar.Visibility = Visibility.Hidden;
                            lbl_con_status1.Visibility = Visibility.Hidden;
                            pbar.Visibility = Visibility.Hidden;
                            btn_select_firmware_file.Visibility = Visibility.Hidden;
                            labelFirmware_rev_val.Visibility = Visibility.Hidden;
                            pbar_cpld.Visibility = Visibility.Hidden;
                            lbl1pbar_cpld.Visibility = Visibility.Hidden;
                        }));
                    }

               
                    Thread.Sleep(50);

                    // Request slot card data
                    for (uint i = 0; i < NUMBER_OF_BOARD_SLOTS; i++)
                    {
                        system_request_device_detection(i);
                        Thread.Sleep(30);
                    }


                controls.slot_header.OpenErrorHandler
                    DefaultOpenErrorHandler(ContentControl control) =>
                    (slot, status_bits) =>
                    {
                        new controls.pnp_error_handler(control, status_bits, new(slot,
                            system_set_sn_to_current_device,
                            system_allow_currently_connected_device));
                    };

                    // grab info needed for each card type
                    for (byte i = 0; i < NUMBER_OF_BOARD_SLOTS; i++)
                    {
                        //   Thread.Sleep(400);
                        switch (card_type[i])
                        {
                            case (Int16)CardTypes.ST_Stepper_type:
                            case (Int16)CardTypes.High_Current_Stepper_Card:
                            case (Int16)CardTypes.High_Current_Stepper_Card_HD:
                            case (Int16)CardTypes.ST_Invert_Stepper_BISS_type:
                            case (Int16)CardTypes.ST_Invert_Stepper_SSI_type:
                            case (Int16)CardTypes.MCM_Stepper_Internal_BISS_L6470:
                            case (Int16)CardTypes.MCM_Stepper_Internal_SSI_L6470:
                            case (Int16)CardTypes.MCM_Stepper_L6470_MicroDB15:
                            case (Int16)CardTypes.MCM_Stepper_LC_HD_DB15:
                                //  we need the counts_per_unit &   nm_per_count  
                                // request Stepper drive params
                                Dispatcher.Invoke(new Action(() =>
                                {
                                    ContentControl st_sel = (ContentControl)FindName("brd_slot_" + (i + 1).ToString());
                                    var ctrl = new controls.stepper(i, new controls.stepper.Callbacks(this, BindProfile("stepper-control"), (obj, brush) =>
                                    {
                                        Border brd = (Border)st_sel.Parent;
                                        brd.BorderBrush = brush;
                                    }), DefaultOpenErrorHandler(st_sel));
                                    st_sel.Content = ctrl;
                                    stepper_windows.Add(new Tuple<controls.stepper, controls.stepper.ConfigurationReadyArgs>(ctrl, new(i)));
                                    slot_header_windows.Add(ctrl.Header);
                                }));
                            
                                //build_stepper_control(i);
                                //byte slot = (byte)(Convert.ToByte(i) + 0x21);
                                //command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_STAGEPARAMS));
                                //bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
                                ////Thread.Sleep(100);  // for some reason we need this delay here 
                                //response_received = false;
                                //send_again_params:
                                //tries++;
                                //usb_write(bytesToSend, 6);
                                //Thread.Sleep(50); // 50
                            
                                //// these values are very important so we need to make sure we get a response
                                //if ((!response_received) && (tries < 50))
                                //    goto send_again_params;


                                // request Jog params
                                //command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_REQ_JOGPARAMS));
                                //bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
                                //usb_write(bytesToSend, 6);
                                //Thread.Sleep(30);

                                //// request Limit params
                                //command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_MOT_REQ_LIMSWITCHPARAMS));
                                //bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
                                //usb_write(bytesToSend, 6);
                                //Thread.Sleep(30);

                                //stepper_uninitialize(i);
                                break;

                        //case (Int16)CardTypes.Servo_type:
                        //    build_servo_control(i);
                        //    request_postion(i);
                        //    break;

                        //case (Int16)CardTypes.Shutter_type:
                        //    build_shutter_controls(i);
                        //    break;

                        //case (Int16)CardTypes.OTM_Dac_type:
                        //    build_otm_dac_controls(i);
                        //    break;

                        //case (Int16)CardTypes.OTM_RS232_type:
                        //    build_otm_rs232_controls(i);
                        //    request_laser_params(1, i);
                        //    break;

                        //case (Int16)CardTypes.Slider_IO_type:
                        //    build_io_control(i);
                        //    request_mirror_postions(i, 0);
                        //    request_mirror_postions(i, 1);
                        //    request_mirror_postions(i, 2);
                        //    break;

                        //case (Int16)CardTypes.Shutter_4_type:
                        //case (Int16)CardTypes.Shutter_4_type_REV6:
                        //    build_shutter_4_controls(i);
                        //    request_shutter_4_state(i);
                        //    break;

                        //case (Int16)CardTypes.Piezo_Elliptec_type:
                        //    build_piezo_elliptec_control(i);
                        //    break;

                        //case (Int16)CardTypes.Piezo_type:
                        //    build_piezo_control(i);
                        //    break;

                        case (Int16)CardTypes.MCM_Flipper_Shutter:
                        case (Int16)CardTypes.MCM_Flipper_Shutter_REVA:
                            Dispatcher.Invoke(new Action(() =>
                            {
                                ContentControl st_sel = (ContentControl)FindName("brd_slot_" + (i + 1).ToString());
                                var callbacks = new controls.USBCallback(this, BindProfile("flipper-shutter-control"));
                                var ctrl = new controls.flipper_shutter(i, callbacks, DefaultOpenErrorHandler(st_sel));
                                st_sel.Content = ctrl;
                                flipper_shutter_windows.Add(ctrl);
                                slot_header_windows.Add(ctrl.Header);
                            }));
                            break;

                        }
                    }

                    Thread.Sleep(30);

                    // Request titles
                    foreach (var header in slot_header_windows)
                    {
                        header.RequestSlotTitle();
                        header.RequestPartNumber();
                    }


                // Request the serial number protection for each slot.
                for (int i = 0; i < NUM_OF_SLOTS; ++i)
                {
                    system_req_device_board((byte)i);
                    Thread.Sleep(15);
                }

                _lutVersion = "None";
                // Request the EFS hw info.
                if (board_type_t != 0)
                {
                    _efs_driver.RequestHeaderInfo();
                    RequestLutLocked();
                    // Request the LUT info
                    var versionFile = _efs_driver.LookupFileAsync(0x03, 0x02).Result;
                    if (versionFile is not null)
                    {
                        try
                        {
                            byte[] versionBytes = versionFile.ReadAsync(32 * 2 + 4, 4).Result;
                            _lutVersion = $"{versionBytes[0]}.{versionBytes[1]}.{BitConverter.ToUInt16(versionBytes, 2)}";
                        }
                        catch { }
                    }
                }
                render_version_info();

                // Request the system dim value for tablet and joystick led's
                byte[] command_2 = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOD_REQ_SYSTEM_DIM));
                    bytesToSend = new byte[6] { command_2[0], command_2[1], 0x00, 0x00, MOTHERBOARD_ID, HOST_ID };
                    usb_write("req_system_dim", bytesToSend, 6);

                }
                catch (Exception)
                {
                    Thread.Sleep(30);
                }
                USB_reading = true;

                // ONLY START LOGGING AFTER ABOVE COMPLETE
                // Start system logging
                if (ENABLE_SYSTEM_LOGGING)
                {
                    byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_START_LOG));
                    bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, MOTHERBOARD_ID, HOST_ID };
                    usb_write("start_log", bytesToSend, 6);
                    Thread.Sleep(50);
                }
            }

        private void usb_disconnect()
        {
            try
            {
                lbl_con_status.Dispatcher.Invoke(new Action(() =>
                {
                    lbl_con_status.Content = "Disconnected";
                    lbl_con_status.Foreground = Brushes.Red;
                    MCM6000_tabs.IsEnabled = false;
                    e_btn_boardIdentify.Visibility = Visibility.Collapsed;
                }));

                _timer1.Interval = 1000;
                _txScheduler.Port.DataReceived -= SerialPort_DataReceived;

                if (_txScheduler.Port.IsOpen == true)
                {
                    _txScheduler.Port.Close();
                }
                IsConnected = false;

                foreach (var v in flipper_shutter_windows)
                {
                    v.CancelRequests();
                }

                this.Dispatcher.Invoke(new Action(() =>
                {
                    // clear all stack panels on Control tab
                    brd_slot_1.Content = null;
                    brd_slot_2.Content = null;
                    brd_slot_3.Content = null;
                    brd_slot_4.Content = null;
                    brd_slot_5.Content = null;
                    brd_slot_6.Content = null;
                    brd_slot_7.Content = null;
                    brd_slot_8.Content = null;
                }));

                stepper_windows.Clear();
                slot_header_windows.Clear();
                flipper_shutter_windows.Clear();
                _efs_driver.Reset();

            }
            catch (Exception)
            {
                // MessageBox.Show(ex.Message);
            }
        }

        private bool HasTouchInput()
        {
            foreach (TabletDevice tabletDevice in Tablet.TabletDevices)
            {
                //Only detect if it is a touch Screen not how many touches (i.e. Single touch or Multi-touch)
                if (tabletDevice.Type == TabletDeviceType.Touch)
                    return true;
            }

            return false;
        }

        byte slot_num_to_update = 0;
        List<controls.slot_header> slot_header_windows = new();
        List<Tuple<controls.stepper, controls.stepper.ConfigurationReadyArgs>> stepper_windows = new();
        List<controls.flipper_shutter> flipper_shutter_windows = new();

        private void _timer1_Elapsed(Object source, ElapsedEventArgs e)
        {
            bool card_detect = false;
            _timer1.Enabled = false;
            try
            {


                if (!IsConnected)
                {
                    usb_connect();
                    Thread.Sleep(100);
                }

                if ((current_main_tab == ControlTab) || (current_main_tab == MesoTab))// && !tabs_removed)
                {
                    if (!ftdi_flag)
                        _timer1.Interval = 15; //15
                    else
                        _timer1.Interval = 15; //25

                    if (slot_num_to_update == NUMBER_OF_BOARD_SLOTS)
                    {
                        // call to see if any slots have an error
                        board_update_request();

                        SolidColorBrush Color = new SolidColorBrush();
                        this.Dispatcher.Invoke(new Action(() =>
                        {
                            // check each slot to see if there is an error, if so make boarder yellow
                            for (int i = 1; i <= NUMBER_OF_BOARD_SLOTS; i++)
                            {
                                byte bit_position = (byte)(1 << (i - 1));

                                if (slot_error == bit_position)
                                {
                                    Color = System.Windows.Media.Brushes.Yellow;

                                    // Border brd_name control tab
                                    string b_name = "_slot_" + i.ToString();
                                    Border brd_name = (Border)this.FindName(b_name);

                                    // stackpanel name control tab
                                    string s_name = "brd_slot_" + i.ToString();
                                    StackPanel sp_name = (StackPanel)this.FindName(s_name);

                                    // meso only has 4 slots for borders
                                    if (i < SLOT5)
                                    {
                                        // Border brd_name meso tab
                                        string b_meso_name = "_meso_border_" + i.ToString();
                                        Border brd_meso_name = (Border)this.FindName(b_meso_name);

                                        // stackpanel name meso tab
                                        string s_meso_name = "sp_meso_" + i.ToString();
                                        StackPanel sp_meso_name = (StackPanel)this.FindName(s_meso_name);

                                        // apply color control tab
                                        brd_name.BorderBrush = Color;
                                        // clear the stack panel control tab
                                        sp_name.Children.Clear();


                                        // apply color messo tab
                                        brd_meso_name.BorderBrush = Color;
                                        // clear the stack panel meso tab
                                        sp_meso_name.Children.Clear();
                                    }
                                }
                            }
                        }));
                        slot_num_to_update++;
                    }
                    else
                    {
                        if (current_main_tab == MesoTab)
                        {
                            request_joystick_data();
                        }

                        while (!card_detect && slot_num_to_update < NUMBER_OF_BOARD_SLOTS)
                        {
                            switch (card_type[slot_num_to_update])
                            {
                                case (Int16)CardTypes.ST_Stepper_type:
                                case (Int16)CardTypes.High_Current_Stepper_Card:
                                case (Int16)CardTypes.High_Current_Stepper_Card_HD:
                                case (Int16)CardTypes.ST_Invert_Stepper_BISS_type:
                                case (Int16)CardTypes.ST_Invert_Stepper_SSI_type:
                                case (Int16)CardTypes.MCM_Stepper_Internal_BISS_L6470:
                                case (Int16)CardTypes.MCM_Stepper_Internal_SSI_L6470:
                                case (Int16)CardTypes.MCM_Stepper_L6470_MicroDB15:
                                case (Int16)CardTypes.MCM_Stepper_LC_HD_DB15:
                                    mcm_stepper_update(slot_num_to_update);
                                    card_detect = true;
                                    break;

                                case (Int16)CardTypes.Servo_type:
                                    request_postion(slot_num_to_update);
                                    card_detect = true;
                                    break;

                                case (Int16)CardTypes.Shutter_type:
                                    request_shutter_state(slot_num_to_update);
                                    card_detect = true;
                                    break;

                                case (Int16)CardTypes.OTM_Dac_type:
                                    card_detect = true;
                                    break;

                                case (Int16)CardTypes.OTM_RS232_type:
                                    request_laser_params(12, slot_num_to_update);
                                    card_detect = true;
                                    break;

                                case (Int16)CardTypes.Slider_IO_type:
                                    // TODO PR request_mirror_postions(slot_num_to_update, ParallelLoopResult thru channels);
                                    //  card_detect = true;
                                    request_mirror_postions(slot_num_to_update, chan_index);
                                    card_detect = true;
                                    chan_index++;
                                    if (chan_index >= 3)
                                        chan_index = 0;
                                    break;

                                case (Int16)CardTypes.Shutter_4_type:
                                case (Int16)CardTypes.Shutter_4_type_REV6:
                                    request_shutter_4_state(slot_num_to_update);
                                    card_detect = true;
                                    break;

                                case (Int16)CardTypes.Piezo_Elliptec_type:
                                    elliptec_update(slot_num_to_update);
                                    card_detect = true;
                                    break;

                                case (Int16)CardTypes.Piezo_type:
                                    card_detect = true;
                                    break;

                                default:
                                    card_detect = false;
                                    break;

                            }
                            foreach (var x in flipper_shutter_windows.Where(x => x.Slot == slot_num_to_update))
                            {
                                x.DispatchPoll();
                            }

                            byte[] command = BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_REQ_PNPSTATUS);
                            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, MOTHERBOARD_ID, HOST_ID };
                            foreach (var header in slot_header_windows.Where(x => x.Slot == slot_num_to_update))
                            {
                                bytesToSend[2] = (byte)header.Slot;
                                usb_write("req_pnp_status", bytesToSend, 6);
                            }
                            slot_num_to_update++;
                        }
                    }

                    // +1 so we can read board_update_request 
                    // if (slot_num_to_update >= NUMBER_OF_BOARD_SLOTS + 1)      
                    if (slot_num_to_update > NUMBER_OF_BOARD_SLOTS)
                    {
                        slot_num_to_update = 0;
                        card_detect = true;
                    }


                }
                else if (current_main_tab == system_tab)
                {
                    _timer1.Interval = 500;
                    if (!in_bootloader)
                        board_update_request();
                }
                else if (current_main_tab == hexapod_tab)
                {
                    _timer1.Interval = 100;
                    hexapod_update_request();
                }
                else if (current_main_tab == stepper_log_tab)
                {
                    _timer1.Interval = log_interval;
                    plot_slot_values();
                }
            }
            finally
            {
                _timer1.Enabled = true;
            }
        }

        private readonly models.UsbUsageProfiler _usbWriteProfiler = new();

        public const int PRIORITY_IMMEDIATE = OutboundSerialMessageScheduler.IMMEDIATE_PRIORITY;
        public const int PRIORITY_NORMAL = 0;
        public const int PRIORITY_LOW = -10;

        private static readonly ManualResetEventSlim _usbWriterThreadExit = new(false);
        private static Thread _usbWriterThread;
        private void UsbWriterThread()
        {
            const int CLEANUP_RATE = 800; // messages / cleanup operation (garbage disposal effectively).
            int cleanup_counter = 0;

            var timeout = TimeSpan.FromMilliseconds(400);
            while (!_usbWriterThreadExit.Wait(0))
            {
                // Periodically wake up to check the exit condition.
                if (!_txScheduler.BlockAndSendScheduledMessage(timeout))
                { continue; }

                if (++cleanup_counter == CLEANUP_RATE)
                {
                    cleanup_counter = 0;
                    _txScheduler.CleanupQueues();
                }
            }
        }

        private void schedule_usb_write(string profile, byte[] bytesToSend, int length, int priority = PRIORITY_NORMAL)
        {
            var inst = _usbWriteProfiler;
            // var profiler = inst.BeginProfiling(profile);

            if (IsConnected)
            {
                try
                {
                    _txScheduler.Send(bytesToSend, 0, length, priority);
                }
                catch (Exception)
                {
                    //MessageBox.Show(ex.Message);
                    Thread.Sleep(100);
                    usb_disconnect();
                }
                finally
                {
                    inst.MarkUsbNotInUse();
                }
            }
            else if (_boot_finished)
            {
                inst.MarkUsbNotInUse();
                usb_connect();
            }
            else
            {
                inst.MarkUsbNotInUse();
            }
        }

        private readonly object _profileLockCV = new();
        private string _profileLock = string.Empty;
        private void SetProfileLock(string value)
        {
            lock (_profileLockCV)
            {
                _profileLock = value;
                Monitor.Pulse(_profileLockCV);
            }
        }
        private void WaitForProfileLock(string profile)
        {
            lock (_profileLockCV)
            {
                while (_profileLock != string.Empty && _profileLock != profile)
                {
                    Monitor.Wait(_profileLockCV);
                }
            }
        }

        private void usb_write(string profile, byte[] bytesToSend, int length)
        {
            WaitForProfileLock(profile);
            var inst = _usbWriteProfiler;
            // var profiler = inst.BeginProfiling(profile);

            if (IsConnected)
            {
                try
                {
                    _txScheduler.SendAsap(bytesToSend, 0, length);
                }
                catch (Exception)
                {
                    //MessageBox.Show(ex.Message);
                    Thread.Sleep(100);
                    usb_disconnect();
                }
                finally
                {
                    inst.MarkUsbNotInUse();
                }
            }
            else if (_boot_finished)
            {
                inst.MarkUsbNotInUse();
                usb_connect();
            }
            else
            {
                inst.MarkUsbNotInUse();
            }
        }

        private void SerialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                do
                {

              
                if (loading_firmware == true)
                {
                    while (_txScheduler.Port.BytesToRead < S70_PAGE_SIZE + 6) { };
                    ext_data = new byte[600];
                    _txScheduler.Port.Read(ext_data, 0, _txScheduler.Port.BytesToRead);
                    firmware_transfer_done.Set();
                }

                header = new byte[6];
                usb_bytes = _txScheduler.Port.BytesToRead;

                if (_txScheduler.Port.BytesToRead > 5)
                {
                    _txScheduler.Port.Read(header, 0, 6);
                }
                else
                    return;

                if ((header[4] & 0x80) != 0)
                {
                    int length = header[2] | header[3] << 8;
                    if (length > 100)
                        return;
                    while (_txScheduler.Port.BytesToRead < length) { };
                    ext_data = new byte[length];
                    _txScheduler.Port.Read(ext_data, 0, length);
                    parse_apt();
                }
                else
                {
                    ext_data = new byte[1] { 0 };
                    parse_apt();
                }

                } while (usb_bytes > 5);
               

            }
            catch (Exception)
            {
            //   MessageBox.Show("Error: " + ex.Message);
            }
        }

        private void parse_apt()
        {
            int command = header[0] | header[1] << 8;
            byte? slot = null;
            if (0x21 <= header[5] && header[5] <= 0x2A)
            {
                slot = (byte)(header[5] - 0x21);
            }
            switch (command)
            {
                case MGMSG_GET_SER_STATUS:
                    OnSerialNumberAck(ext_data[0]);
                    break;

                case MGMSG_MOT_GET_STATUSUPDATE:
                    stepper_status_update();
                    break;

                case MGMSG_MCM_GET_STATUSUPDATE:
                    mcm_stepper_status_update();
                    break;

                case MGMSG_MCM_GET_STEPPER_LOG:
                    stepper_log_update();
                    break;

                case MGMSG_MOT_GET_ENCCOUNTER:
                    if (card_type[ext_data[0]] == (Int16)CardTypes.Piezo_Elliptec_type)
                        elliptec_status_update();
                    else
                        stepper_status_update();
                    break;

                case MGMSG_MOD_GET_JOYSTICK_INFO:
                    usb_device_info();
                    break;

                case Thorlabs.APT.MGMSG_MOD_GET_JOYSTICK_CONTROL:
                    {
                        byte address = ext_data[0];
                        bool isOut = ext_data[1] > 0;
                        byte controlNumber = ext_data[2];
                        uint usage = (uint)(ext_data[5] | (ext_data[6] << 8) | (ext_data[3] << 16) | (ext_data[4] << 32));
                        usb_control_info(address, isOut, controlNumber, usage);
                    }
                    break;

                case MGMSG_MOD_GET_JOYSTICKS_MAP_IN:
                    usb_device_mapping_in();
                    break;

                case MGMSG_MOD_GET_JOYSTICKS_MAP_OUT:
                    usb_device_mapping_out();
                    break;
                case MGMSG_MCM_GET_JOYSTICK_DATA:
                    meso_get_joystick_data_selected_slot();
                    break;

                case MGMSG_MCM_MOT_GET_LIMSWITCHPARAMS:
                    stepper_get_limits_params();
                    break;

                case MGMSG_MCM_GET_HOMEPARAMS:
                    stepper_get_home_params();
                    break;

                case MGMSG_MCM_GET_STAGEPARAMS:
                    response_received = true;
                    stepper_get_drive_params();
                    break;

                case MGMSG_MOT_GET_JOGPARAMS:
                    stepper_get_jog_params();
                    break;

                case MGMSG_MOT_GET_MFF_OPERPARAMS:
                    servo_get_params();
                    break;

                case MGMSG_MOT_GET_BUTTONPARAMS:
                    servo_status_update();
                    break;

                case MGMSG_BOARD_GET_STATUSUPDATE:
                    board_update();
                    break;

                case MGMSG_MCM_GET_HEX_POSE:
                    hexapod_update();
                    break;

                case MGMSG_MOT_GET_DCPIDPARAMS:
                    //hexapod_update();
                    stepper_get_pid_params();
                    break;

                case MGMSG_MOT_GET_SOL_STATE:
                    if (slot is null) { break; }
                    switch ((CardTypes)card_type[slot.Value])
                    {
                        case CardTypes.Shutter_4_type:
                        case CardTypes.Shutter_4_type_REV6:
                        case CardTypes.MCM_Flipper_Shutter:
                        case CardTypes.MCM_Flipper_Shutter_REVA:
                            shutter_4_get_state();
                            break;

                        default:
                            shutter_get_state();
                            break;
                    }
                    break;

                case MGMSG_MCM_GET_SHUTTERPARAMS:
                    if (slot is null) { break; }
                    switch ((CardTypes)card_type[slot.Value])
                    {
                        case CardTypes.Shutter_4_type:
                        case CardTypes.Shutter_4_type_REV6:
                        case CardTypes.MCM_Flipper_Shutter:
                        case CardTypes.MCM_Flipper_Shutter_REVA:
                            shutter_4_get_params(slot.Value, (CardTypes)card_type[slot.Value]);
                            break;

                        default:
                            shutter_get_params();
                            break;
                    }
                    break;

                case MGMSG_MCM_GET_INTERLOCK_STATE:
                    shutter_get_interlock_state();
                    flipper_shutter_get_interlock_state();
                    break;

                case MGMSG_LA_GET_PARAMS:
                    get_laser_params();
                    break;

                case MGMSG_GET_DEVICE:
                    system_tab_get_devices();
                    ow_request_device_callback(ext_data);
                    slot_get_slot_part_number(ext_data[0],
                        Encoding.ASCII.GetString(ext_data.Skip(14).Take(16).ToArray()));
                    _system_sn_waiter.Set();
                    _system_last_read_devices = ext_data;
                    break;

                case MGMSG_GET_DEVICE_BOARD:
                    system_get_device_board(ext_data[0], BitConverter.ToUInt64(ext_data,2));
                    break;

                case MGMSG_MCM_GET_SYNC_MOTION_PARAM:
                    synchonized_motion_get_state();
                    break;

                case MGMSG_MOD_GET_SYSTEM_DIM:
                    system_dim_value_req();
                    break;

                case MGMSG_MCM_GET_MIRROR_STATE:
                    mirror_position_get();
                    flipper_shutter_get_mirror_state();
                    break;

                case MGMSG_MCM_GET_MIRROR_PARAMS:
                    mirrors_get_params();
                    break;

                case MGMSG_GET_STORE_POSITION:
                    if (card_type[ext_data[0]] == (Int16)CardTypes.Piezo_Elliptec_type)
                        elliptec_get_saved_positions();
                    else
                        stepper_get_saved_positions();
                    break;

                case MGMSG_GET_STORE_POSITION_DEADBAND:
                    stepper_get_saved_positions_deadband();
                    break;

                case MGMSG_BL_GET_FIRMWAREVER:
                    string temp_S;
                    temp_S = "Bootloader Version: "
                        + Convert.ToString(ext_data[2]) + "."
                       + Convert.ToString(ext_data[3]);

                    this.Dispatcher.Invoke(new Action(() =>
                    {
                        labelFirmware_rev_val.Content = temp_S;
                    }));
                    break;

                case MGMSG_CPLD_UPDATE:
                    cpld_data_recieved_flag = 1;
                    cpld_data_recieved = header[3];
                    break;

                case MGMSG_MCM_POST_LOG:
                    log();
                    break;
                case MGMSG_MCM_GET_ENABLE_LOG:
                    system_tab_get_enable_logging();
                    break;
                
                case MGMSG_MCM_PIEZO_GET_PRAMS:
                    piezo_get_params();
                    break;

                case MGMSG_MCM_PIEZO_GET_VALUES:
                    piezo_get_values();
                    break;

                case MGMSG_MCM_PIEZO_GET_LOG:
                    piezo_log_update();
                    break;

                case MGMSG_MCM_PIEZO_GET_PID_PARMS:
                    piezo_get_pid_params();
                    break;
               
                case MGMSG_MCM_PIEZO_GET_MODE:
                    break;

                case MGMSG_MCM_GET_ALLOWED_DEVICES:
                    byte[] data_cpy = (byte[])ext_data.Clone();
                    if (system_requested_allowed_devices)
                    {
                        system_requested_allowed_devices = false;
                        Dispatcher.Invoke(new Action(() =>
                            View_allowed_devices_Callback(data_cpy[0], data_cpy)
                        ));
                    }
                    _system_allowed_devices_waiter.Set();
                    _system_last_read_allowed = data_cpy;
                    break;

                case MGMSG_OW_GET_PROGRAMMING:
                    Array.Copy(ext_data, m_ow_response_buffer, 3);
                    m_ow_response_ready.Release();
                    break;
                case MGMSG_OW_GET_PROGRAMMING_SIZE:
                    Array.Copy(ext_data, m_ow_response_buffer, 2);
                    m_ow_response_ready.Release();
                    break;

                case MGMSG_MCM_GET_DEVICE_DETECTION:
                    system_request_device_detection_Callback(ext_data[0], ext_data[2] > 0);
                    break;

                case Thorlabs.APT.MGMSG_MCM_GET_SLOT_TITLE:
                    byte[] str = new byte[16];
                    Array.Copy(ext_data, 2, str, 0, 16);
                    slot_get_slot_title(ext_data[0], str);
                    break;

                case Thorlabs.APT.MGMSG_MCM_GET_PNPSTATUS:
                    slot_get_pnp_status_update(ext_data[0], BitConverter.ToUInt32(ext_data, 2));
                    break;
                case Thorlabs.APT.MGMSG_MCM_EFS_GET_FILEDATA:
                case Thorlabs.APT.MGMSG_MCM_EFS_GET_FILEINFO:
                    _efs_driver.OnDataRecieved((ushort)command, ext_data);
                    break;
                case Thorlabs.APT.MGMSG_MCM_EFS_GET_HWINFO:
                    string ident = Encoding.ASCII.GetString(ext_data, 0, 3);
                    byte version = ext_data[3];
                    uint page_size = BitConverter.ToUInt16(ext_data, 4);
                    uint pages_supported = BitConverter.ToUInt16(ext_data, 6);
                    uint max_files = BitConverter.ToUInt16(ext_data, 16);
                    uint files_remaining = BitConverter.ToUInt16(ext_data, 18);
                    uint pages_remaining = BitConverter.ToUInt16(ext_data, 20);
                    _efs_driver.UpdateHeaderInfo(
                        ident,
                        version,
                        page_size,
                        pages_supported,
                        max_files,
                        files_remaining,
                        pages_remaining
                    );
                    break;
                case Thorlabs.APT.MGMSG_MCM_LUT_GET_LOCK:
                    LUTSetUILock(header[2] != 0);
                    break;
                case Thorlabs.APT.MGMSG_MOD_GET_CHANENABLESTATE:
                    flipper_shutter_get_channel_enable();
                    break;
                case Thorlabs.APT.MGMSG_MCM_GET_SPEED_LIMIT:
                    if (slot is not null)
                    {
                        onrecv_get_speed_limit(slot.Value, ext_data[2], BitConverter.ToUInt32(ext_data, 3));
                    }
                    break;
            }
        }

        private void slot_get_pnp_status_update(byte slot, uint status_flags)
        {
            foreach (var obj in slot_header_windows)
            {
                obj.UpdateStatusBits(slot, status_flags);
            }
        }

        private void slot_get_slot_title(byte slot, byte[] title_binary)
        {
            string title = Encoding.ASCII.GetString(title_binary);

            var args = new controls.slot_header.TitleChangedArgs();
            args.Slot = slot;
            args.Title = title;

            foreach (var v in slot_header_windows)
            {
                v.InvokeTitleChanged(this, args);
            }
        }

        private void slot_get_slot_part_number(byte slot, string name)
        {
            var args = new controls.slot_header.GotStageNameArgs();
            args.Slot = slot;
            args.Name = name;

            foreach (var v in slot_header_windows)
            {
                v.InvokeGotStageName(this, args);
            }
        }

        private void MCM6000_tabs_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                cb_piezo_plot_disable();
                TabItem current_main_tab_temp = MCM6000_tabs.SelectedValue as TabItem;
                if (e.Source is TabControl) // This is a soultion of those problem.
                {
                    if (current_main_tab_temp == current_main_tab)
                        return;

                    else if (current_main_tab_temp.Name == "USB_host")
                    {
                        usb_host_update();
                    }
                    else if (current_main_tab_temp.Name == "Log_tab")
                    {
                        btRefresh_log_file_Click(null, null);
                    }
                    else if (current_main_tab_temp.Name == "system_tab")
                    {
                        system_tab_init();
                    }
                    else if (current_main_tab_temp.Name == "vritual_motion")
                    {
                        motion_tab_init();
                    }

                    current_main_tab = MCM6000_tabs.SelectedValue as TabItem;


                }
            }
            catch (Exception)
            {
            }
        }

        private void btUpdate_Click(object sender, RoutedEventArgs e)
        {
            MessageBoxResult result = MessageBox.Show("This will put the device in Bootloader Mode. Continue only if you have new firmware hex file. Do you want to continue?", "Firmware Update Question", MessageBoxButton.YesNoCancel);
            if (result == MessageBoxResult.Yes)
            {
                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_GET_UPDATE_FIRMWARE));
                byte[] bytesToSend = new byte[6] { command[0], command[1], 1, 0x00, MOTHERBOARD_ID, HOST_ID };
                usb_write("update_firmware", bytesToSend, 6);
            }
            else
            {

            }

        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            Application.Current.Shutdown();
        }

        private Window _diagWindow = null;
        private view_models.UsbUsageProfilerVM _diagVm = null;
        private void Button_Diag_Clicked(object sender, RoutedEventArgs args)
        {
            if (_diagWindow is not null)
            {
                _diagWindow.Show();
                return;
            }

            _diagVm ??= new()
                {
                    Model = _usbWriteProfiler,
                };

            _diagWindow = new Window
            {
                Title = "Diagnostic Data",
                Height = 600,
                Width = 800,
                Content = new views.CommsDiagnosticsView()
                {
                    DataContext = _diagVm,
                }
            };
            _diagWindow.Closed += (_, _) => { _diagWindow = null; };

            _diagWindow.Show();
        }

        private void Button_Identify_Clicked(object sender, RoutedEventArgs args)
        {
            // Safety hook
            if (loading_cpld)
                return;

            byte[] bytesToSend = new byte[6] { 0, 0, 0xFF, 0, Thorlabs.APT.Address.MOTHERBOARD, Thorlabs.APT.Address.HOST };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOD_IDENTIFY), 0, bytesToSend, 0, 2);

            usb_write("identify", bytesToSend, bytesToSend.Length);
        }

        private void system_dim_value_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {

            byte dim_value = (byte)system_dim_value.Value;

            // MGMSG_MOD_SET_JOYSTICK_MAP_IN
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOD_SET_SYSTEM_DIM));
            byte[] bytesToSend = new byte[] { command[0], command[1], dim_value, 0, Convert.ToByte(MOTHERBOARD_ID), HOST_ID };

            usb_write("sytem_dim_changed", bytesToSend, 6);
            Thread.Sleep(25);
        }

        private void request_joystick_data()
        {
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_JOYSTICK_DATA));
            byte[] bytesToSend = new byte[] { command[0], command[1], 0, 0, Convert.ToByte(MOTHERBOARD_ID), HOST_ID };

            usb_write("request_joystick_data", bytesToSend, 6);
            Thread.Sleep(25);
        }

        private void system_dim_value_req()
        {
            lbl_con_status.Dispatcher.Invoke(new Action(() =>
            {
                system_dim_value.Value = header[2];
            }));

        }

        private void CartesianChart_Loaded_1(object sender, RoutedEventArgs e)
        {

        }

        private void Restart_processor_Click(object sender, RoutedEventArgs e)
        {
            Restart_processor(true);
        }

        private void clear_focus()
        {
            System.Threading.Thread.Sleep(150); // was 250
            TraversalRequest request = new TraversalRequest(FocusNavigationDirection.Next);
            MoveFocus(request);
        }

        private void clear_focus_fast()
        {
            System.Threading.Thread.Sleep(10);
            TraversalRequest request = new TraversalRequest(FocusNavigationDirection.Next);
            MoveFocus(request);
        }

        public struct POINT
        {
            public int X;
            public int Y;
        }

        /// <summary>
        /// Returns the bytes that will save a given command to the controller.
        /// </summary>
        /// <param name="destination">Destination of the save command.</param>
        /// <param name="channel">Channel to save. Usually 0.</param>
        /// <param name="command_to_save">The command to save.</param>
        private byte[] save_command(byte destination, ushort command_to_save, ushort channel = 0)
        {
            byte[] rt = new byte[10];

            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_SET_EEPROMPARAMS), 0, rt, 0, 2);
            rt[2] = 4;
            rt[3] = 0;
            rt[4] = (byte)(0x80 | destination);
            rt[5] = Thorlabs.APT.Address.HOST;
            Array.Copy(BitConverter.GetBytes(channel), 0, rt, 6, 2);
            Array.Copy(BitConverter.GetBytes(command_to_save), 0, rt, 8, 2);

            return rt;
        }
        
        private void onrecv_get_speed_limit(byte slot, byte channel_mask, uint value)
        {
            foreach (var v in stepper_windows)
            {
                if (v.Item2.Slot != slot)
                { continue; }

                // (sbenish) Allowing multi-masking 
                if (channel_mask == 0)
                {
                    v.Item2.OptionalSpeedLimit.HardwareSpeedLimit = value;
                }
                if ((channel_mask & 0x01) != 0)
                {
                    v.Item2.OptionalSpeedLimit.AbsoluteMovementSpeedLimit = value;
                }
                if ((channel_mask & 0x02) != 0)
                {
                    v.Item2.OptionalSpeedLimit.RelativeMovementSpeedLimit = value;
                }
                if ((channel_mask & 0x04) != 0)
                {
                    v.Item2.OptionalSpeedLimit.JoggingMovementSpeedLimit = value;
                }
                if ((channel_mask & 0x08) != 0)
                {
                    v.Item2.OptionalSpeedLimit.VelocityMovementSpeedLimit = value;
                }

                v.Item1.PostConfiguration(v.Item2);
            }
        }

        [DllImport("User32.dll")]
        private static extern bool SetCursorPos(int X, int Y);
        
        [DllImport("user32.dll")]
        private static extern bool GetCursorPos(out POINT lpPoint);
    }
}
