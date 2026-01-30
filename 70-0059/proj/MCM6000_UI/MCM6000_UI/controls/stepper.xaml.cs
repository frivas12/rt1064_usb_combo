using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
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

    public enum StepperStopMode
    {
        Default = 0,
        Hard = 1,
        Soft = 2,
    }

    /// <summary>
    /// Interaction logic for stepper_view.xaml
    /// </summary>
    public partial class stepper : UserControl, INotifyPropertyChanged
    {
        public class Callbacks : USBCallback
        {
            public delegate void SetBorderBrushDelegate(object class_param, Brush brush);

            private readonly SetBorderBrushDelegate _set_border_brush;

            public void SetBoarderBrush(Brush brush)
            {
                _set_border_brush(_class_param, brush);
            }

            public Callbacks(object class_param, WriteUSBDelegate write_usb, SetBorderBrushDelegate set_boarder_brush)
                : base(class_param, write_usb)
            {
                _set_border_brush = set_boarder_brush;
            }
        }


        public enum Direction
        {
            CCW = 0,
            CW = 1,
        }


        // BEGIN    Private Fields
        private double _coefficient; // linear -> nm/count, rotational -> degrees/count
        private double _counts_per_unit;
        private readonly byte _slot_id;
        private bool _initialized;
        private readonly SemaphoreSlim _initalization_done = new(0);
        private bool _reversed;
        private bool _is_rotational;
        private bool _prefers_soft_stop;

        // Positions that get bound.
        private int _encoder_position;
        private int _encoder_raw_position;
        private int _stepper_position;

        // Radio Button Binds
        private bool _moving_ccw;
        private bool _limits_hard_ccw;
        private bool _limits_soft_ccw;
        private bool _moving_cw;
        private bool _limits_hard_cw;
        private bool _limits_soft_cw;

        // Homing
        private bool _homing;
        private bool _homed;

        // Speed Limits
        private bool _speedFeaturesEnabled = false;
        private uint _speedLimitHardware = 0;

        // Soft Stop
        private bool _softStopFeatureEnabled = false;
        private StepperStopMode _stopMode = StepperStopMode.Default;

        // Control Flag
        private bool _controls_enabled;

        // Encoder Error
        private string _encoder_error_text;
        private SolidColorBrush _encoder_error_brush;

        // Border Brush
        private SolidColorBrush _border_brush;

        private readonly Callbacks _callbacks;

        private const string STEPPER_TYPE_STRING = "Stepper";
        private const string DEFAULT_ENCODER_ERROR_TEXT = "None";
        private static readonly SolidColorBrush DEFAULT_ENCODE_ERROR_BRUSH = new(Colors.White);

        // Speed
        private byte _fast_move_percentage = 100;
        private byte _slow_move_percentage = 40;

        // Setting Saved Positions
        private byte _saved_pos_set_sel_box = 0;
        private int[] _saved_positions = new int[10];
        private const int CLEAR_POSITION = 2147483647;

        // Setting Button Direction
        private bool[] _setting_move_direction = [true, false];    // Normal, Reversed

        // END      Private Fields

        // BEGIN    Non-Bound Public Properties

        public Brush OnSavedPositionBrush { get; set; } = new SolidColorBrush(Colors.Red);

        // END      Non-Bound Public Properties

        // BEGIN    Bound Properties
        public string UnitString => GetUnitString(_is_rotational);

        public string EncoderPosition
        {
            get { return StringifyEncoder(_encoder_position); }
        }
        public string EncoderToolTip
        {
            get
            {
                return string.Format("Encoder (um): {0}\nRaw Encoder (um): {1}\nError: {2}",
                    StringifyEncoder(_encoder_position),
                    StringifyEncoder(_encoder_raw_position),
                    _encoder_error_text);
            }
        }
        public string StepperPosition
        {
            get { return StringifyStepper(_stepper_position); }
        }

        public bool MovingCCW { get { return _moving_ccw; } }
        public bool MovingCW { get { return _moving_cw; } }
        public bool HardLimitCCW { get { return _limits_hard_ccw; } }
        public bool HardLimitCW { get { return _limits_hard_cw; } }
        public bool SoftLimitCCW { get { return _limits_soft_ccw; } }
        public bool SoftLimitCW { get { return _limits_soft_cw; } }

        public bool NotHomed { get { return !_homing && !_homed; } }
        public bool Homing { get { return _homing; } }
        public bool Homed { get { return _homed; } }

        public bool IsSpeedFeaturesEnabled => ControlsEnabled && _speedFeaturesEnabled;
        public bool IsSoftStopFeatureEnabled => ControlsEnabled && _softStopFeatureEnabled;

        public byte FastMovePercentage
        {
            get { return _fast_move_percentage; }
            set
            {
                _fast_move_percentage = value;
                PropertyChanged?.Invoke(this, new(nameof(FastMovePercentage)));
                if (_slow_move_percentage > _fast_move_percentage)
                {
                    SlowMovePercentage = value;
                }
            }
        }
        public byte SlowMovePercentage
        {
            get { return _slow_move_percentage; }
            set
            {
                if (value <= _fast_move_percentage)
                {
                    _slow_move_percentage = value;
                }
                PropertyChanged?.Invoke(this, new(nameof(SlowMovePercentage)));
            }
        }
        public bool ControlsEnabled { get { return _controls_enabled; } }
        public string ControlToggle
        {
            get
            {
                return (_controls_enabled) ? "Disable" : "Enable";
            }
        }

        public SolidColorBrush EncoderErrorBrush { get { return _encoder_error_brush; } }
        public SolidColorBrush BorderBrush { get { return _border_brush; } }

        public readonly slot_header Header;

        public bool[] UIMoveDirection
        { 
            get { return _setting_move_direction; }
        }
        public int UIMoveDirection_Selected
        { 
            get { return Array.IndexOf(_setting_move_direction, true); }
        }

        public StepperStopMode StopMode
        {
            get => _stopMode;
            set
            {
                _stopMode = value;
                PropertyChanged?.Invoke(this, new(nameof(StopMode)));
            }
        }

        // END      Bound Properties

        // BEGIN    Events

        public class OnStatusUpdateArgs
        {
            public byte Slot;
            public int StepperCount;
            public int EncoderCount;
            public int EncoderRawCount;
            public byte CurrentStoredPosition;
            public bool OnCCWHardLimit;
            public bool OnCWHardLimit;
            public bool OnCCWSoftLimit;
            public bool OnCWSoftLimit;
            public bool MovingCCW;
            public bool MovingCW;
            public bool IsHoming;
            public bool IsHomed;
            public bool MagnetTooClose;
            public bool MagnetTooFar;
            public bool MagnetNotReady;
            public bool ChannelEnabled;
            public bool MotorConnected;
            public byte StoredPositionOn;
        }

        public enum MovementChannel
        {
            ABSOLUTE,
            RELATIVE,
            JOGGING,
        }

        /// <summary>
        /// Speed Limits parameters to update.
        /// Parameters that are null do not update the control.
        /// </summary>
        public class SpeedLimitConfiguration
        {
            public uint? HardwareSpeedLimit
            { get; set; } = null;
            public uint? AbsoluteMovementSpeedLimit
            { get; set; } = null;
            public uint? RelativeMovementSpeedLimit
            { get; set; } = null;
            public uint? JoggingMovementSpeedLimit
            { get; set; } = null;
            public uint? VelocityMovementSpeedLimit
            { get; set; } = null;

            public bool All()
            {
                return HardwareSpeedLimit is not null &&
                    AbsoluteMovementSpeedLimit is not null &&
                    RelativeMovementSpeedLimit is not null &&
                    JoggingMovementSpeedLimit is not null &&
                    VelocityMovementSpeedLimit is not null;
            }

            public bool Any()
            {
                return HardwareSpeedLimit is not null ||
                    AbsoluteMovementSpeedLimit is not null ||
                    RelativeMovementSpeedLimit is not null ||
                    JoggingMovementSpeedLimit is not null ||
                    VelocityMovementSpeedLimit is not null;
            }
        }

        public class SoftStopConfiguration
        {
            public bool? Enabled
            { get; set; } = null;
            public bool? PrefersSoftStop
            { get; set; } = null;
        }
        
        /// <summary>
        /// Configuration parameters to update.
        /// Parameters that are null do not update the control.
        /// </summary>
        public class ConfigurationReadyArgs
        {
            public int Slot
            { get; }

            public double? NMPerCount
            { get; set; } = null;
            public double? CountsPerUnit
            { get; set; } = null;
            public int? JogStepSize
            { get; set; } = null;
            public bool? StepperReversed
            { get; set; } = null;
            public int[] SavedPositions
            { get; set; } = null;
            public bool? IsRotational
            { get; set; } = null;
            public bool? PrefersSoftStop
            { get; set; } = null;

            public SpeedLimitConfiguration OptionalSpeedLimit
            { get; } = new();

            public bool? OptionalSoftStopSupported
            { get; set; } = null;

            public bool All()
            {
                return NMPerCount is not null &&
                    CountsPerUnit is not null &&
                    JogStepSize is not null &&
                    StepperReversed is not null &&
                    SavedPositions is not null &&
                    IsRotational is not null &&
                    PrefersSoftStop is not null;
            }

            public bool Any()
            {
                return NMPerCount is not null ||
                    CountsPerUnit is not null ||
                    JogStepSize is not null ||
                    StepperReversed is not null ||
                    SavedPositions is not null ||
                    IsRotational is not null ||
                    PrefersSoftStop is not null;
            }


            public ConfigurationReadyArgs(int slot)
            {
                Slot = slot;
            }
        }

        public delegate void OnStatusUpdateHandler(object sender, OnStatusUpdateArgs args);
        public delegate void ConfigurtionReadyHandler(object sender, ConfigurationReadyArgs args);

        public event OnStatusUpdateHandler? OnStatusUpdate;
        public event ConfigurtionReadyHandler? ConfigurationReady;
        public event PropertyChangedEventHandler? PropertyChanged;

        public void InvokeStatusUpdate(object sender, OnStatusUpdateArgs args) { OnStatusUpdate?.Invoke(sender, args); }

        public void PostConfiguration(ConfigurationReadyArgs args) { if (args.Any()) { ConfigurationReady?.Invoke(this, args); } }

        // END      Events

        // BEGIN    Constructors
        public stepper(byte slot_id, Callbacks callbacks, slot_header.OpenErrorHandler open_error_handler)
        {
            _callbacks = callbacks;

            // Bind Events
            OnStatusUpdate += OnStatusUpdateEvent;
            ConfigurationReady += ConfigurationReadyEvent;

            // Set Default Values
            _coefficient = 0;
            _counts_per_unit = 0;
            _initialized = false;
            _encoder_position = 0;
            _encoder_raw_position = 0;
            _stepper_position = 0;
            _moving_ccw = false;
            _moving_cw = false;
            _limits_hard_ccw = false;
            _limits_hard_cw = false;
            _limits_soft_ccw = false;
            _limits_soft_cw = false;
            _homing = false;
            _homed = false;
            _controls_enabled = false;
            _slot_id = slot_id;
            _reversed = false;

            Header = new slot_header(slot_id, STEPPER_TYPE_STRING, callbacks, open_error_handler);

            for (int i = 0; i < _saved_positions.Length; ++i)
            {
                _saved_positions[i] = CLEAR_POSITION;
            }

            InitializeComponent();
            DataContext = this;

            Uninitialize();
            SetSettingsVisibility(false);

            // Setting top header
            e_ccl_header.Content = Header;
        }
        // END      Constructors

        // BEGIN    Public Interface
        /**
         * Sent to request a status update for the controller.
         */
        public void RequestUpdate()
        {
            byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
            byte[] bytes_to_send = new byte[6] { 0, 0, 0, 0, SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_REQ_STATUSUPDATE), 0, bytes_to_send, 0, 2);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        /**
         * Sent to begin a homing move.
         */
        public void Home()
        {
            byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
            byte[] bytes_to_send = new byte[6] { 0, 0, 0, 0, SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_MOVE_HOME), 0, bytes_to_send, 0, 2);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        /**
         * Sent to zero the encoder count (for relative encoders).
         */
        public void ZeroEncoder()
        {
            byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
            byte[] bytes_to_send = new byte[12] { 0, 0, 6, 0, (byte)(SLOT_ADDRESS | 0x80), Thorlabs.APT.Address.HOST,
                _slot_id, 0, 0, 0, 0, 0};
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_SET_ENCCOUNTER), 0, bytes_to_send, 0, 2);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        /**
         * Sent to stop the motor.
         */
        public void Stop(StepperStopMode mode = StepperStopMode.Default)
        {
            byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
            byte[] bytes_to_send = new byte[6] { 0, 0, 0, (byte)(mode), SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_MOVE_STOP), 0, bytes_to_send, 0, 2);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        /**
         * Sent to hard-stop the motor.
         */
        public void HardStop() => Stop(StepperStopMode.Hard);

        /**
         * Sent to soft-stop the motor.
         */
        public void SoftStop() => Stop(StepperStopMode.Soft);

        /**
         * Sent to set the current position as the soft limit for the given direction.
         */
        public void SetSoftLimit(Direction dir) => SetSoftLimits((byte)((dir == Direction.CCW) ? 1 : 2));

        /**
         * Sent to clear the soft limits.
         */
        public void ClearSoftLimits() => SetSoftLimits(3);

        /**
         * Sent to move the stage to an encoder position.
         */
        public void Goto(int position)
        {
            byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
            byte[] bytes_to_send = new byte[12] { 0, 0, 6, 0, (byte)(SLOT_ADDRESS | 0x80), Thorlabs.APT.Address.HOST,
            _slot_id, 0, 0, 0, 0, 0};
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_MOVE_ABSOLUTE), 0, bytes_to_send, 0, 2);
            Array.Copy(BitConverter.GetBytes(position), 0, bytes_to_send, 8, 4);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        /**
         * Sent to jog in the specified direction.
         */
        public void Jog(Direction dir) => Jog((byte)((dir == Direction.CCW ^ (UIMoveDirection_Selected == 1)) ? 0 : 1));

        /**
         * Sent to set the jog step size.
         */
        public void SaveJogStepSize(int step_size)
        {
            byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
            byte[] bytes_to_send = new byte[28] { 0, 0, 22, 0, (byte)(SLOT_ADDRESS | 0x80), Thorlabs.APT.Address.HOST,
                _slot_id, 0,        // Chan Ident
                2, 0,               // Jog Mode - Single-Step Jogging
                0, 0, 0, 0,         // Jog Step Size
                0, 0, 0, 0,         // Min Velocity (unused)
                0, 0, 0, 0,         // Acceleration (unused)
                0, 0, 0, 0,         // Max Velocity (unused)
                0, 0 };             // Sop mode (unused)
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_SET_JOGPARAMS), 0, bytes_to_send, 0, 2);
            Array.Copy(BitConverter.GetBytes(step_size), 0, bytes_to_send, 10, 4);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        /**
         * Sent to move to a set position.
         */
        public void MoveToPosition(int saved_position_index)
        {
            byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
            byte[] bytes_to_send = new byte[6] { 0, 0, _slot_id, (byte)(saved_position_index), SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_SET_GOTO_STORE_POSITION), 0, bytes_to_send, 0, 2);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        /**
         * Sent to move by a certain amount in encoder counts.
         */
        public void MoveBy(int delta)
        {
            byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
            byte[] bytes_to_send = new byte[12] { 0, 0, 6, 0, (byte)(SLOT_ADDRESS | 0x80), Thorlabs.APT.Address.HOST,
            _slot_id, 0, 0, 0, 0, 0};
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_MOT_MOVE_BY), 0, bytes_to_send, 0, 2);
            Array.Copy(BitConverter.GetBytes(delta), 0, bytes_to_send, 8, 4);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        /**
         * Sent to begin a velocity move.
         */
        public void MoveDir(Direction dir, byte velocity_percent)
        {
            byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
            byte DIRECTION = (byte)((dir == Direction.CCW ^ (UIMoveDirection_Selected == 1)) ? 0 : 1);
            byte[] bytes_to_send = new byte[6] { 0, 0, DIRECTION, velocity_percent, SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_MOVE_VELOCITY), 0, bytes_to_send, 0, 2);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        /**
         * Sets the saved position local, but does not write it to the controller (see FlushSavedPositions).
         * Saved position index is in the integer range [0, 9].
         */
        public void SetSavedPosition(byte saved_position_index, int encoder_position)
        {
            if (saved_position_index >= 10)
            {
                throw new ArgumentException("Saved position index too large");
            }
            _saved_positions[saved_position_index] = encoder_position;
        }

        /**
         * Sets the saved position locally from the current encoder position.
         * Saved position index is in the integer range [0, 9].
         */
        public void SetSavedPositionFromCurrentPosition(byte saved_position_index) => SetSavedPosition(saved_position_index, _encoder_position);

        /**
         * Clears the saved position at the given index locally.
         * Saved position index is in the integer range [0, 9].
         */
        public void ClearSavedPosition(byte saved_position_index) => SetSavedPosition(saved_position_index, CLEAR_POSITION);

        /**
         * Writes all of the saved positions to the controller.
         */
        public void FlushSavedPositions()
        {
            byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
            byte[] bytes_to_send = new byte[48];
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_SET_STORE_POSITION), 0, bytes_to_send, 0, 2);
            bytes_to_send[2] = 42;
            bytes_to_send[3] = 0;
            bytes_to_send[4] = (byte)(SLOT_ADDRESS | 0x80);
            bytes_to_send[5] = Thorlabs.APT.Address.HOST;
            bytes_to_send[6] = _slot_id;
            bytes_to_send[7] = 0;
            for (int i = 0; i < 10; ++i)
            {
                Array.Copy(BitConverter.GetBytes(_saved_positions[i]), 0, bytes_to_send, 8 + 4 * i, 4);
            }

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }


        /// <summary>
        /// Sets the speed limit of the movement type.
        /// </summary>
        /// <param name="movementType"></param>
        /// <param name="percentage"></param>
        public void SetSpeedLimit(MovementChannel movementType, double percentage)
        {
            byte bitset;
            switch(movementType)
            {
                default:
                    return;

                case MovementChannel.ABSOLUTE:
                    bitset = 0x01;
                    break;

                case MovementChannel.RELATIVE:
                    bitset = 0x02;
                    break;

                case MovementChannel.JOGGING:
                    bitset = 0x04;
                    break;
            }

            uint value = (uint)(percentage / 100 * _speedLimitHardware);
            value = Math.Min(_speedLimitHardware, Math.Max(value, 1));


            byte SLOT_ADDRESS = (byte)((Thorlabs.APT.Address.SLOT_1 + _slot_id) | 0x80);
            byte[] bytes_to_send = new byte[13] { 0, 0, 7, 0, SLOT_ADDRESS, Thorlabs.APT.Address.HOST, _slot_id, 0, bitset, 0, 0, 0, 0 };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_SET_SPEED_LIMIT), 0, bytes_to_send, 0, 2);
            Array.Copy(BitConverter.GetBytes(value), 0, bytes_to_send, 9, 4);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        // END      Public Interface

        // BEGIN    Private Interface

        /**
         * Translates the integer encoder value into a double value in um.
         */
        private string StringifyEncoder(int encoder_val) => StringifyEncoder((double)encoder_val);
        private string StringifyEncoder(double encoder_val)
        {
            double encoder = EncoderCountsToUnits(encoder_val);
            return encoder.ToString("0,0.000");
        }
        /**
         * Translates the integer stepper value into a double value in um.
         */
        private string StringifyStepper(int stepper_val)
        {
            double encoder_counts = stepper_val / _counts_per_unit;
            return StringifyEncoder(encoder_counts);
        }

        /**
         * Updates the paramters upon a stepper update.
         */
        private void OnStatusUpdateEvent(object sender, OnStatusUpdateArgs args)
        {
            if (args.Slot == _slot_id)
            {
                // Update internal variables.
                _stepper_position = args.StepperCount;
                _encoder_position = args.EncoderCount;
                _encoder_raw_position = args.EncoderRawCount;

                // Update radio buttons.
                _limits_hard_ccw = args.OnCCWHardLimit;
                _limits_hard_cw = args.OnCWHardLimit;
                _moving_ccw = args.MovingCCW;
                _moving_cw = args.MovingCW;
                _limits_soft_ccw = args.OnCCWSoftLimit;
                _limits_soft_cw = args.OnCWSoftLimit;

                // Update homed status.
                _homing = args.IsHoming;
                _homed = !args.IsHoming && args.IsHomed;

                // Update encoder error.
                if (args.MagnetTooClose)
                {
                    _encoder_error_text = "Magnet too close";
                    _encoder_error_brush = new(Colors.Orange);
                }
                else if (args.MagnetTooFar)
                {
                    _encoder_error_text = "Magnet too far or wheel out";
                    _encoder_error_brush = new(Colors.Yellow);
                }
                else if (args.MagnetNotReady)
                {
                    _encoder_error_text = "Magnet not ready";
                    _encoder_error_brush = new(Colors.Turquoise);
                }
                else
                {
                    _encoder_error_text = DEFAULT_ENCODER_ERROR_TEXT;
                    _encoder_error_brush = DEFAULT_ENCODE_ERROR_BRUSH;
                }

                // Update Slot Controls.
                ChangeSlotControls(args.ChannelEnabled);

                // Update border brush.
                if (_limits_hard_ccw || _limits_hard_cw || _limits_soft_ccw || _limits_soft_cw || !args.MotorConnected)
                {
                    // On limit or no connect
                    _border_brush = new(Colors.Red);
                }
                else if (_moving_ccw || _moving_cw)
                {
                    // Moving
                    _border_brush = new(Colors.Blue);
                }
                else
                {
                    _border_brush = new(Colors.Green);
                }

                // Check for initialization.
                if (args.MotorConnected && !_initialized)
                {
                    Initialize();
                }
                else if (!args.MotorConnected && _initialized)
                {
                    Uninitialize();
                }

                // Invoke property changed.
                PropertyChanged?.Invoke(this, new(nameof(EncoderPosition)));
                PropertyChanged?.Invoke(this, new(nameof(EncoderToolTip)));
                PropertyChanged?.Invoke(this, new(nameof(StepperPosition)));
                PropertyChanged?.Invoke(this, new(nameof(MovingCCW)));
                PropertyChanged?.Invoke(this, new(nameof(MovingCW)));
                PropertyChanged?.Invoke(this, new(nameof(HardLimitCCW)));
                PropertyChanged?.Invoke(this, new(nameof(HardLimitCW)));
                PropertyChanged?.Invoke(this, new(nameof(SoftLimitCCW)));
                PropertyChanged?.Invoke(this, new(nameof(SoftLimitCW)));
                PropertyChanged?.Invoke(this, new(nameof(NotHomed)));
                PropertyChanged?.Invoke(this, new(nameof(Homing)));
                PropertyChanged?.Invoke(this, new(nameof(Homed)));

                _callbacks.SetBoarderBrush(_border_brush);

                Dispatcher.Invoke(new Action(() => SetPositionBorderColors(args.StoredPositionOn)));
            }
        }

        private void SetPositionBorderColors(byte stored_position_on)
        {
            for (int i = 0; i < 6; ++i)
            {
                Border dec = (Border)e_grd_top.FindName("e_brd_stored_pos_" + (i + 1).ToString());
                dec.BorderBrush = (i == stored_position_on) ? OnSavedPositionBrush : null;
            }
        }

        private void ChangeSlotControls(bool controls_enabled)
        {
            if (_controls_enabled != controls_enabled)
            {
                _controls_enabled = controls_enabled;

                Task.Run(() =>
                {
                    byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
                    byte[] bytes_to_send = new byte[6] { 0, 0, _slot_id, (byte)((controls_enabled) ? 1 : 0), SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
                    Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOD_SET_CHANENABLESTATE), 0, bytes_to_send, 0, 2);

                    _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
                });

                PropertyChanged?.Invoke(this, new(nameof(ControlsEnabled)));
                PropertyChanged?.Invoke(this, new(nameof(IsSpeedFeaturesEnabled)));
                PropertyChanged?.Invoke(this, new(nameof(IsSoftStopFeatureEnabled)));
                PropertyChanged?.Invoke(this, new(nameof(ControlToggle)));
            }
        }

        private void Initialize()
        {

            Dispatcher.Invoke(() =>
            {
                e_grd_top.IsEnabled = true;
            });
            Task.Run(async () =>
            {
                await RequestParameters();
                _initialized = true;
            });
        }

        private void Uninitialize()
        {
            Dispatcher.Invoke(() =>
            {
                e_grd_top.IsEnabled = false;
            });
            _initialized = false;
        }

        private void SetSoftLimits(byte mode)
        {
            /**
             * 1 |  Set CCW Limit
             * 2 |  Set CW Limit
             * 3 |  Clear Soft Limits
             */
            byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
            byte[] bytes_to_send = new byte[6] { 0, 0, mode, 0, SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_SET_SOFT_LIMITS), 0, bytes_to_send, 0, 2);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        private void Jog(byte direction)
        {
            /**
             * 0 |  CCW
             * 1 |  CW
             */
            byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
            byte[] bytes_to_send = new byte[6] { 0, 0, 0, direction, SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_MOVE_JOG), 0, bytes_to_send, 0, 2);

            _callbacks.WriteUSB(bytes_to_send, bytes_to_send.Length);
        }

        private Task RequestParameters()
        {
            byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + _slot_id);
            byte[] bytes_to_send = new byte[9];
            bytes_to_send[4] = SLOT_ADDRESS;
            bytes_to_send[5] = Thorlabs.APT.Address.HOST;

            // Begin fixed-header requests
            bytes_to_send[2] = 0;
            bytes_to_send[3] = 0;

            // Also gets the hardware speed.
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_REQ_STAGEPARAMS), 0, bytes_to_send, 0, 2);

            _callbacks.WriteUSB(bytes_to_send, 6);

            // Also gets the jog speed.
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_REQ_JOGPARAMS), 0, bytes_to_send, 0, 2);
            _callbacks.WriteUSB(bytes_to_send, 6);

            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_REQ_STORE_POSITION), 0, bytes_to_send, 0, 2);
            _callbacks.WriteUSB(bytes_to_send, 6);

            // Begin dynamic-dynamic requests
            bytes_to_send[4] |= (byte)0x80;

            //  Begin 3-length extended data.
            Array.Copy(BitConverter.GetBytes((ushort)3), 0, bytes_to_send, 2, 2);

            //      Begin speed-limit requests.
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_REQ_SPEED_LIMIT), 0, bytes_to_send, 0, 2);
            Array.Copy(BitConverter.GetBytes((ushort)_slot_id), 0, bytes_to_send, 6, 2);

            //          Absolute
            bytes_to_send[8] = 0x01;
            _callbacks.WriteUSB(bytes_to_send, 9);

            //          Relative
            bytes_to_send[8] = 0x02;
            _callbacks.WriteUSB(bytes_to_send, 9);

            //          Velocity
            bytes_to_send[8] = 0x08;
            _callbacks.WriteUSB(bytes_to_send, 9);

            return _initalization_done.WaitAsync();
        }

        private void ConfigurationReadyEvent(object sender, ConfigurationReadyArgs args)
        {
            if (args.Slot != _slot_id)
            { return; }

            List<Action> dispatcherActions = [];
            bool releaseConfiguration = false;

            static bool ForwardProperty<T>(ref T dest, T? src)
                where T : struct
            {
                if (src.HasValue)
                {
                    dest = src.Value;
                }

                return src.HasValue;
            }

            if (args.Any())
            {
                ForwardProperty(ref _coefficient, args.NMPerCount);
                ForwardProperty(ref _counts_per_unit, args.CountsPerUnit);
                ForwardProperty(ref _reversed, args.StepperReversed);
                if (ForwardProperty(ref _is_rotational, args.IsRotational))
                {
                    dispatcherActions.Add(() =>
                    {
                        PropertyChanged?.Invoke(this, new(nameof(UnitString)));
                    });
                }
                ForwardProperty(ref _prefers_soft_stop, args.PrefersSoftStop);

                if (args.JogStepSize.HasValue)
                {
                    dispatcherActions.Add(() =>
                    {
                        e_tbx_jog_step_size.Text = EncoderCountsToUnits(args.JogStepSize.Value).ToString("0,0.000");
                    });
                }

                releaseConfiguration = args.All();
            }

            if (args.OptionalSpeedLimit.Any())
            {
                bool hardwareChanged = ForwardProperty(ref _speedLimitHardware, args.OptionalSpeedLimit.HardwareSpeedLimit);

                static void UpdateTextbox(TextBox box, uint value, uint limit)
                {
                    box.Text = Math.Round(100.0 * value / limit).ToString();
                }

                if (hardwareChanged || args.OptionalSpeedLimit.AbsoluteMovementSpeedLimit.HasValue)
                {
                    dispatcherActions.Add(() => UpdateTextbox(e_tbx_absolute_speed, args.OptionalSpeedLimit.AbsoluteMovementSpeedLimit.Value, _speedLimitHardware));
                }
                if (hardwareChanged || args.OptionalSpeedLimit.RelativeMovementSpeedLimit.HasValue)
                {
                    dispatcherActions.Add(() => UpdateTextbox(e_tbx_relative_speed, args.OptionalSpeedLimit.RelativeMovementSpeedLimit.Value, _speedLimitHardware));
                }
                if (hardwareChanged || args.OptionalSpeedLimit.JoggingMovementSpeedLimit.HasValue)
                {
                    dispatcherActions.Add(() => UpdateTextbox(e_tbx_jog_step_speed, args.OptionalSpeedLimit.JoggingMovementSpeedLimit.Value, _speedLimitHardware));
                }

                // (sbenish) The set will handle clamping of the current value to the speed limit.
                FastMovePercentage = FastMovePercentage;
                SlowMovePercentage = SlowMovePercentage;

                _speedFeaturesEnabled = args.OptionalSpeedLimit.All();

                dispatcherActions.Add(() =>
                {
                    PropertyChanged?.Invoke(this, new(nameof(IsSpeedFeaturesEnabled)));
                });
            }

            if (args.OptionalSoftStopSupported.HasValue)
            {
                _softStopFeatureEnabled = args.OptionalSoftStopSupported.Value;

                dispatcherActions.Add(() =>
                {
                    PropertyChanged?.Invoke(this, new(nameof(IsSoftStopFeatureEnabled)));
                });
            }


            if (dispatcherActions.Any())
            {
                Dispatcher.Invoke(() =>
                {
                    foreach (Action action in dispatcherActions)
                    {
                        action.Invoke();
                    }
                });
            }

            if (_initalization_done.CurrentCount == 0 && releaseConfiguration)
            {
                _initalization_done.Release();
            }
        }

        private int UnitsToEncoderCounts(double value)
        {
            // [1/1000th of a °] * 1000 * [unit/°] = [unit]
            // [µm] * [nm/µm] * [unit/nm] = [unit]
            return (int)Math.Round(value * 1e3 / _coefficient);
        }

        private double EncoderCountsToUnits(double value)
        {
            // [unit] * [°/unit] / 1000 = [°]
            // [unit] * [nm/unit] * [µm/nm] = [µm]
            return value * _coefficient / 1000;
        }

        private void SetSettingsVisibility(bool settings_visible)
        {
            Dispatcher.Invoke(() =>
            {
                e_grd_settings.Visibility = settings_visible ? Visibility.Visible : Visibility.Collapsed;
                e_grd_top.Visibility = settings_visible ? Visibility.Collapsed : Visibility.Visible;
            });
        }

        private void UpdateSetSavedPosSelected(Border border, int new_index)
        {
            Border? old_border = null;
            if (_saved_pos_set_sel_box > 0)
            {
                string target_name = "e_brd_conf_saved_pos_" + _saved_pos_set_sel_box.ToString();
                object? hit = e_grd_settings.FindName(target_name);
                old_border = hit as Border;
            }

            _saved_pos_set_sel_box = (byte)new_index;

            Dispatcher.Invoke(() =>
            {
                if (old_border is not null)
                {
                    old_border.BorderBrush = new SolidColorBrush(Colors.White);
                }
                border.BorderBrush = OnSavedPositionBrush;
            });
        }


        private void OnPropertyChanged(object sender,  PropertyChangedEventArgs args)
        {
            Debug.Assert(ReferenceEquals(this, sender));

            switch(args.PropertyName)
            {
                case nameof(ControlsEnabled):
                    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(IsSpeedFeaturesEnabled)));
                    break;
            }
        }
        private static string GetUnitString(bool isRotational)
        { return isRotational ? "°" : "µm"; }

        /// <summary>
        /// Hard or soft-stops based on the UI.
        /// </summary>
        private void UIStop()
        {
            Stop(StopMode);
        }

        // END      Private Interface

        // BEGIN    Button Bindings



        private void Button_ClearSoftLimits_Clicked(object sender, RoutedEventArgs args) => ClearSoftLimits();
        private void Button_SetCCWSoftLimits_Clicked(object sender, RoutedEventArgs args) => SetSoftLimit(Direction.CCW);
        private void Button_SetCWSoftLimits_Clicked(object sender, RoutedEventArgs args) => SetSoftLimit(Direction.CW);

        private void Button_MoveCCWFast_Down(object sender, MouseButtonEventArgs args) => MoveDir(Direction.CCW, FastMovePercentage);
        private void Button_MoveCWFast_Down(object sender, MouseButtonEventArgs args) => MoveDir(Direction.CW, FastMovePercentage);
        private void Button_MoveCCWSlow_Down(object sender, MouseButtonEventArgs args) => MoveDir(Direction.CCW, SlowMovePercentage);
        private void Button_MoveCWSlow_Down(object sender, MouseButtonEventArgs args) => MoveDir(Direction.CW, SlowMovePercentage);
        private void Button_MoveDir_Up(object sender, MouseButtonEventArgs args) => UIStop(); // TODO: May need to change this in the future.

        private void Button_MoveCCWFast_Down(object sender, TouchEventArgs args) => MoveDir(Direction.CCW, FastMovePercentage);
        private void Button_MoveCWFast_Down(object sender, TouchEventArgs args) => MoveDir(Direction.CW, FastMovePercentage);
        private void Button_MoveCCWSlow_Down(object sender, TouchEventArgs args) => MoveDir(Direction.CCW, SlowMovePercentage);
        private void Button_MoveCWSlow_Down(object sender, TouchEventArgs args) => MoveDir(Direction.CW, SlowMovePercentage);
        private void Button_MoveDir_Up(object sender, TouchEventArgs args) => UIStop(); // TODO: May need to change this in the future.

        private void Button_JogCCW_Clicked(object sender, RoutedEventArgs args) => Jog(Direction.CCW);
        private void Button_JogCW_Clicked(object sender, RoutedEventArgs args) => Jog(Direction.CW);
        private void Button_Home_Clicked(object sender, RoutedEventArgs args) => Home();
        private void Button_StageEnabled_Clicked(object sender, RoutedEventArgs args) => ChangeSlotControls(!_controls_enabled);
        private void Button_Stop_Clicked(object sender, RoutedEventArgs args) => UIStop();
        private void Button_Zero_Clicked(object sender, RoutedEventArgs args) => ZeroEncoder();
        private void Button_Goto_Clicked(object sender, RoutedEventArgs args)
        {
            float num;
            try
            {
                num = float.Parse(e_tbx_goto.Text);
            }
            catch { return; }
            int enc = UnitsToEncoderCounts(num);
            Goto(enc);
        }
        private void Button_MoveBy_Clicked(object sender, RoutedEventArgs args)
        {
            float num;
            try
            {
                num = float.Parse(e_tbx_move_by.Text);
            }
            catch { return; }
            int enc = UnitsToEncoderCounts(num);
            MoveBy(enc);
        }
        private void Button_JogSize_Clicked(object sender, RoutedEventArgs args)
        {
            float num;
            try
            {
                num = float.Parse(e_tbx_jog_step_size.Text);
            }
            catch { return; }
            int enc = UnitsToEncoderCounts(num);
            SaveJogStepSize(enc);
        }
        private void Button_JogSpeed_Clicked(object sender, RoutedEventArgs args)
        {
            float num;
            try
            {
                num = float.Parse(e_tbx_jog_step_speed.Text);
            }
            catch { return; }
            SetSpeedLimit(MovementChannel.JOGGING, num);
        }
        private void Button_AbsoluteSpeed_Clicked(object sender, RoutedEventArgs args)
        {
            float num;
            try
            {
                num = float.Parse(e_tbx_absolute_speed.Text);
            }
            catch { return; }
            SetSpeedLimit(MovementChannel.ABSOLUTE, num);
        }
        private void Button_RelativeSpeed_Clicked(object sender, RoutedEventArgs args)
        {
            float num;
            try
            {
                num = float.Parse(e_tbx_relative_speed.Text);
            }
            catch { return; }
            SetSpeedLimit(MovementChannel.RELATIVE, num);
        }
        private void Button_GotoSavedPosition_Clicked(object sender, RoutedEventArgs args)
        {
            Button? btn = sender as Button;
            if (btn is not null)
            {
                byte saved_index;
                try
                {
                    saved_index = (byte)(int.Parse((string)btn.Content) - 1);
                }
                catch { return; }
                MoveToPosition(saved_index);
            }
        }
        private void Button_OpenSettings_Clicked(object? sender, RoutedEventArgs args) => SetSettingsVisibility(true);
        private void Button_CloseSettings_Clicked(object? sender, RoutedEventArgs args) => SetSettingsVisibility(false);
        private void Button_SelectSetSavedPos_Clicked(object? sender, RoutedEventArgs args)
        {
            Border? border = ((FrameworkElement)sender).Parent as Border;
            if (border is not null && border.Name.Length > 0 && int.TryParse(border.Name.Substring(border.Name.Length - 1), out int num))
            {
                UpdateSetSavedPosSelected(border, num);

                Dispatcher.Invoke(() =>
                {
                    e_tbx_set_saved_position.Text = StringifyEncoder(_saved_positions[num - 1]);
                });
            }
        }
        private void Button_ClearSavedPos_Clicked(object? sender, RoutedEventArgs args) => ClearSavedPosition((byte)(_saved_pos_set_sel_box - 1));
        private void Button_SetSavedPos_Clicked(object? sender, RoutedEventArgs args)
        {
            if (double.TryParse(e_tbx_set_saved_position.Text, out double um_pos))
            {
                int enc_pos = (int)(um_pos * 1e3 / _coefficient); 
                SetSavedPosition((byte)(_saved_pos_set_sel_box - 1), enc_pos);
            }
        }
        private void Button_CopySavedPos_Clicked(object? sender, RoutedEventArgs args)
        {
            SetSavedPosition((byte)(_saved_pos_set_sel_box - 1), _encoder_position);
            Dispatcher.Invoke(() =>
            {
                e_tbx_set_saved_position.Text = StringifyEncoder(_saved_positions[_saved_pos_set_sel_box - 1]);
            });
        }
        private void Button_SaveSavedPos_Clicked(object? sender, RoutedEventArgs args) => FlushSavedPositions();
    
    }
}
