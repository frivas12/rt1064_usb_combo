using MCM6000_UI.controls.drivers;
using ReactiveUI;
using System;
using System.CodeDom;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reactive;
using System.Runtime.CompilerServices;
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
using TinyMessenger;

namespace MCM6000_UI.controls
{
    /// <summary>
    /// Interaction logic for flipper_shutter.xaml
    /// </summary>
    public partial class flipper_shutter : UserControl
    {

        #region Communication Events
        public struct ChannelEnableEventArgs
        {
            public byte slot;
            public int channel;
            public bool is_enabled;

            public readonly byte[] SerializeSet()
            {
                byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + slot);
                var rt = new byte[6]
                { 0, 0, (byte)channel, (byte)(is_enabled ? 1 : 0), SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOD_SET_CHANENABLESTATE), 0, rt, 0, 2);
                return rt;
            }

            public static ChannelEnableEventArgs DeserializeGet(byte[] data)
            {
                if (data.Length != 6)
                { throw new ArgumentException(); }

                return new ChannelEnableEventArgs()
                {
                    slot = (byte)(data[5] - Thorlabs.APT.Address.SLOT_1),
                    channel = data[2],
                    is_enabled = data[3] != 0,
                };
            }

            public static byte[] SerializeRequest(byte slot, int channel)
            {
                byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + slot);
                var rt = new byte[6]
                { 0, 0, (byte)channel, 0, SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOD_REQ_CHANENABLESTATE), 0, rt, 0, 2);
                return rt;
            }
        };

        public struct MirrorStateEventArgs
        {
            public byte slot;
            public int channel;
            public byte mirror_state;

            public readonly bool IsIn()
            { return mirror_state == 1; }

            public readonly bool IsOut()
            { return mirror_state == 0; }

            public readonly bool IsUnknown()
            { return mirror_state == 2; }

            public readonly bool IsStateInvalid()
            { return mirror_state > 2; }
        }

        public struct InterlockStateEventArgs
        {
            public byte slot;
            public bool is_interlock_engaged;
        }

        public struct ShutterParamsEventArgs
        {
            public byte slot;
            public int channel;
            public IShutterDriver.States power_up_state;
            public IShutterDriver.Types type;
            public IShutterDriver.TriggerModes external_trigger_mode;
            public double duty_cycle_percentage_pulse;
            public double duty_cycle_percentage_hold;
            public TimeSpan pulse_width;
            public bool open_uses_negative_voltage;
            public TimeSpan holdoff_time;

            public readonly byte[] SerializeSet()
            {
                var rt = new byte[6 + 19];

                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_SET_SHUTTERPARAMS), 0, rt, 0, 2);
                Array.Copy(BitConverter.GetBytes((ushort)(rt.Length - 6)), 0, rt, 2, 2);
                rt[4] = (byte)(Thorlabs.APT.Address.SLOT_1 + slot | 0x80);
                rt[5] = Thorlabs.APT.Address.HOST;
                Array.Copy(BitConverter.GetBytes((ushort)channel), 0, rt, 6, 2);
                rt[8] = (byte)power_up_state;
                rt[9] = (byte)type;
                rt[10] = (byte)external_trigger_mode;
                rt[11] = (byte)(Math.Ceiling(pulse_width.TotalMilliseconds / 10)); // Rounding up
                Array.Copy(BitConverter.GetBytes((uint)(1900 * duty_cycle_percentage_pulse / 100)), 0, rt, 12, 4);
                Array.Copy(BitConverter.GetBytes((uint)(1900 * duty_cycle_percentage_hold / 100)), 0, rt, 16, 4);
                rt[20] = (byte)(open_uses_negative_voltage ? 1 : 0);
                Array.Copy(BitConverter.GetBytes((uint)Math.Ceiling(holdoff_time.TotalMilliseconds * 10)), 0, rt, 21, 4); // Stored in 100s of microseconds.  Rounding up.
                return rt;
            }

            public static ShutterParamsEventArgs DeserializeGet(byte[] data)
            {
                if (data.Length < 6 + 14)
                { throw new ArgumentException(); }

                var rt = new ShutterParamsEventArgs()
                {
                    slot = (byte)(data[5] - Thorlabs.APT.Address.SLOT_1),
                    channel = BitConverter.ToUInt16(data, 6),
                    power_up_state = (IShutterDriver.States)data[7],
                    type = (IShutterDriver.Types)data[8],
                    external_trigger_mode = (IShutterDriver.TriggerModes)data[9],
                    pulse_width = TimeSpan.FromMilliseconds(10 * data[10]),
                    duty_cycle_percentage_pulse = BitConverter.ToUInt32(data, 12) / 1900d * 100,
                    duty_cycle_percentage_hold = BitConverter.ToUInt32(data, 16) / 1900d * 100,
                };

                if (data.Length >= 6 + 15)
                {
                    rt.open_uses_negative_voltage = data[20] != 0;
                }
                else
                {
                    rt.open_uses_negative_voltage = false;
                }

                if (data.Length >= 6 + 19)
                {
                    rt.holdoff_time = TimeSpan.FromMilliseconds((double)BitConverter.ToUInt32(data, 21) / 10);
                }
                else
                {
                    rt.holdoff_time = TimeSpan.Zero;
                }

                return rt;
            }

            public static byte[] SerializeRequest(byte slot, int channel)
            {
                byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + slot);
                var rt = new byte[6]
                { 0, 0, (byte)channel, 0, SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_REQ_SHUTTERPARAMS), 0, rt, 0, 2);
                return rt;
            }
        }

        public struct SolenoidStateEventArgs
        {
            public byte slot;
            public int channel;
            public bool solenoid_on;

            public readonly byte[] SerializeSet()
            {
                byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + slot);
                var rt = new byte[6]
                { 0, 0, (byte)channel, (byte)(solenoid_on ? 0x01 : 0x02), SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_SET_SOL_STATE), 0, rt, 0, 2);
                return rt;
            }

            public static SolenoidStateEventArgs DeserializeGet(byte[] data)
            {
                if (data.Length != 6)
                { throw new ArgumentException(); }

                return new SolenoidStateEventArgs()
                {
                    slot = (byte)(data[5] - Thorlabs.APT.Address.SLOT_1),
                    channel = data[2],
                    solenoid_on = data[3] == 1,
                };
            }

            public static byte[] SerializeRequest(byte slot, int channel)
            {
                byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + slot);
                var rt = new byte[6]
                { 0, 0, (byte)channel, 0, SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_REQ_SOL_STATE), 0, rt, 0, 2);
                return rt;
            }
        }

        public void HandleEvent(ChannelEnableEventArgs args)
        {
            if (args.slot != Slot)
            { return; }
            if (IsShutterChannel(args.channel))
            {
                int index = args.channel - ShutterChannels.Item1;
                if (_shutterBuilders[index].IsReady())
                {
                    if (ShutterControllers[index].Driver is ShutterDriver driver)
                    {
                        driver.HandleEvent(args);
                    }
                }
                else
                {
                    _shutterBuilders[index].Add(args);
                    TryBuildShutterDriver(index);
                }
            }
            else if (IsSioChannel(args.channel))
            {
                int index = args.channel - SioChannels.Item1;
                if (_sioBuilders[index].IsReady())
                {
                    if (Sios[index].Driver is SioDriver driver)
                    {
                        driver.HandleEvent(args);
                    }
                }
                else
                {
                    _sioBuilders[index].Add(args);
                    TryBuildSioDriver(index);
                }
            }
        }
        public void HandleEvent(MirrorStateEventArgs args)
        {
            if (args.slot != Slot)
            { return; }
            if (!IsSioChannel(args.channel))
            { return; }

            int index = args.channel - SioChannels.Item1;
            if (_sioBuilders[index].IsReady())
            {
                if (Sios[index].Driver is SioDriver driver)
                {
                    driver.HandleEvent(args);
                }
            } else
            {
                _sioBuilders[index].Add(args);
                TryBuildSioDriver(index);
            }
        }
        // public void HandleEvent(InterlockStateEventArgs args);
        public void HandleEvent(ShutterParamsEventArgs args)
        {
            if (args.slot != Slot)
            { return; }
            if (!IsShutterChannel(args.channel))
            { return; }

            int index = args.channel - ShutterChannels.Item1;
            if (_shutterBuilders[index].IsReady())
            {
                if (ShutterControllers[index].Driver is ShutterDriver driver)
                {
                    driver.HandleEvent(args);
                }
            }
            else
            {
                _shutterBuilders[index].Add(args);
                TryBuildShutterDriver(index);
            }
        }
        public void HandleEvent(SolenoidStateEventArgs args)
        {
            if (args.slot != Slot)
            { return; }
            if (!IsShutterChannel(args.channel))
            { return; }

            int index = args.channel - ShutterChannels.Item1;
            if (_shutterBuilders[index].IsReady())
            {
                if (ShutterControllers[index].Driver is ShutterDriver driver)
                {
                    driver.HandleEvent(args);
                }
            }
            else
            {
                _shutterBuilders[index].Add(args);
                TryBuildShutterDriver(index);
            }
        }
        #endregion




        public byte Slot => _slot;

        public slot_header Header
        { get; }

        public vms.TTLOutputVM[] Sios
        { get; }

        public vms.ShutterControllerVM[] ShutterControllers
        { get; }

        public vms.ShutterConfiguratorVM[] ShutterConfigurators
        { get; }

        public flipper_shutter(byte slot, USBCallback callbacks,
            slot_header.OpenErrorHandler open_error_handler)
        {
            InitializeComponent();

            Header = new(slot, FLIPPER_SHUTTER_TYPE_STRING, callbacks, open_error_handler);

            _slot = slot;
            _callbacks = callbacks;

            Sios = new vms.TTLOutputVM[SioChannels.Item2 - SioChannels.Item1 + 1];
            ShutterControllers = new vms.ShutterControllerVM[ShutterChannels.Item2 - ShutterChannels.Item1 + 1];
            ShutterConfigurators= new vms.ShutterConfiguratorVM[ShutterChannels.Item2 - ShutterChannels.Item1 + 1];
            _sioBuilders = new SioBuilderCacheEntry[Sios.Length];
            _shutterBuilders = new ShutterBuilderCacheEntry[ShutterControllers.Length];

            // Initialize Sio structures
            for (int i = 0; i < Sios.Length; ++i)
            {
                Sios[i] = new vms.TTLOutputVM();
            }

            for (int i = 0; i < ShutterControllers.Length; ++i)
            {
                ShutterControllers[i] = new();
                ShutterConfigurators[i] = new();
            }

            bool is_loaded = false;
            Header.PropertyChanged += (s, e) =>
            {
                slot_header h = (slot_header)s;

                if (e.PropertyName == nameof(h.IsOkay))
                {
                    if (h.IsOkay == is_loaded)
                    { return; }
                    if (h.IsOkay)
                    {
                        Task.Run(() =>
                        {
                            for (int i = SioChannels.Item1; i <= SioChannels.Item2; ++i)
                            {
                                _sioBuilders[i - SioChannels.Item1] = new SioBuilderCacheEntry()
                                { ChannelEnable = null, MirrorState = null };

                                SioDriver.DispatchRequests(slot, i, callbacks, Token);
                            }
                            for (int i = ShutterChannels.Item1; i <= ShutterChannels.Item2; ++i)
                            {
                                _shutterBuilders[i - ShutterChannels.Item1] = new()
                                { ChannelEnable = null, Params = null, SolenoidState = null };

                                ShutterDriver.DispatchParameterRequest(slot, i, callbacks, Token);
                                ShutterDriver.DispatchStateRequest(slot, i, callbacks, Token);
                            }
                        });
                        is_loaded = true;
                    }
                    else
                    {
                        for (int i = SioChannels.Item1; i <= SioChannels.Item2; ++i)
                        {
                            var driver = Sios[i].Driver;
                            Sios[i].Driver = null;
                        }
                        for (int i = ShutterChannels.Item1; i <= ShutterChannels.Item2; ++i)
                        {
                            var driver = ShutterControllers[i].Driver;
                            ShutterConfigurators[i].Driver = null;
                            ShutterControllers[i].Driver = null;
                        }

                        is_loaded = false;
                    }
                }
            };


            this.DataContext = this;
        }

        public void CancelRequests()
        {
            lock (_cancelLock)
            {
                _cancel?.Cancel();
                _cancel?.Dispose();
                _cancel = null;
            }
        }

        public void DispatchPoll() => Task.Run(() =>
        {
            foreach (var driver in Sios.Select(x => x.Driver).OfType<SioDriver>())
            {
                driver.Request(Token);
            }
            foreach (var driver in ShutterControllers.Select(x => x.Driver).OfType<ShutterDriver>())
            {
                driver.RequestState(Token);
            }
        }, Token);


        private readonly byte _slot;
        private readonly USBCallback _callbacks;

        private readonly SioBuilderCacheEntry[] _sioBuilders;
        private readonly ShutterBuilderCacheEntry[] _shutterBuilders;


        private const string FLIPPER_SHUTTER_TYPE_STRING = "Flipper-Shutter";
        private static readonly Tuple<int, int> SioChannels = Tuple.Create(4, 7);
        private static readonly Tuple<int, int> ShutterChannels = Tuple.Create(0, 3);

        private readonly object _cancelLock = new();
        private CancellationTokenSource _cancel = null;

        private CancellationToken Token
        {
            get
            {
                lock (_cancelLock)
                {
                    _cancel ??= new();
                    return _cancel.Token;
                }
            }
        }


        private static bool IsSioChannel(int channel)
        { return channel >= SioChannels.Item1 && channel <= SioChannels.Item2; }

        private static bool IsShutterChannel(int channel)
        { return channel >= ShutterChannels.Item1 && channel <= ShutterChannels.Item2; }

        private void TryBuildSioDriver(int index)
        {
            if (Sios[index].Driver is not null)
            { return; }
            if (!_sioBuilders[index].IsReady())
            { return; }

            Dispatcher.Invoke(() =>
            {
                Sios[index].Driver = _sioBuilders[index].Build(_callbacks);
            });
        }

        private void TryBuildShutterDriver(int index)
        {
            if (ShutterControllers[index].Driver is not null)
            { return; }
            if (!_shutterBuilders[index].IsReady())
            { return; }

            Dispatcher.Invoke(() =>
            {
                var driver = _shutterBuilders[index].Build(_callbacks);
                ShutterControllers[index].Driver = driver;
                ShutterConfigurators[index].Driver = driver;
            });
        }

        private class SioDriver : ReactiveObject, ITTLOutputDriver
        {
            public bool Level
            {
                get
                {
                    lock (_lock)
                    {
                        return _is_mirror_state_IN;
                    }
                }
                private set
                {
                    bool diff;
                    lock (_lock)
                    {
                        diff = value != _is_mirror_state_IN;
                        _is_mirror_state_IN = value;
                    }
                    if (diff)
                    {
                        this.RaisePropertyChanged();
                    }
                }
            }

            public bool IsEnabled
            {
                get
                {
                    lock (_lock)
                    {
                        return _is_enabled;
                    }
                }
                private set
                {
                    bool diff;
                    lock (_lock)
                    {
                        diff = value != _is_enabled;
                        _is_enabled = value;
                    }
                    if (diff)
                    {
                        this.RaisePropertyChanged();
                    }
                }
            }

            public int Channel
            { get; }

            public Task SetLevelAsync(bool level, CancellationToken token) => Task.Run(() =>
            {
                var currentLevel = Level;
                if (currentLevel == level)
                { return; }

                byte[] rt = Set(Header,
                    new MirrorStatePayload()
                    { is_state_IN = level, });
                _callbacks.WriteUSB(rt, rt.Length);
            }, token);

            public Task SetEnableAsync(bool enable, CancellationToken token) => Task.Run(() =>
            {
                var currentEnable = IsEnabled;
                if (currentEnable == enable)
                { return; }

                byte[] rt = Set(Header, new ChannelEnabledPayload() { is_enabled = enable, });
                _callbacks.WriteUSB(rt, rt.Length);
            }, token);

            public void HandleEvent(ChannelEnableEventArgs args)
            {
                IsEnabled = args.is_enabled;
            }

            public void HandleEvent(MirrorStateEventArgs args)
            {
                Level = args.IsIn();
            }

            private static Task WriteUsbAsync(USBCallback callback, byte[] msg, CancellationToken token) => Task.Run(() => callback.WriteUSB(msg, msg.Length), token);
            public static void DispatchRequests(byte slot, int channel, USBCallback callback, CancellationToken token)
            {
                var header = new MsgHeader { slot = slot, channel = channel };

                byte[] msg = RequestChannelEnabled(header);
                WriteUsbAsync(callback, msg, token).Wait(token);
                token.ThrowIfCancellationRequested();

                msg = RequestMirrorState(header);
                WriteUsbAsync(callback, msg, token).Wait(token);
                token.ThrowIfCancellationRequested();
            }
            public void Request(CancellationToken token)
            {
                try
                {
                    DispatchRequests(_slot, Channel, _callbacks, token);
                }
                catch 
                { }
            }


            public SioDriver(USBCallback callback, ChannelEnableEventArgs chan, MirrorStateEventArgs mirro)
            {
                if (chan.slot != mirro.slot)
                { throw new ArgumentException("slot and mirror arguments have different slots");  }
                if (chan.channel != mirro.channel)
                { throw new ArgumentException("channel and mirror arguments have different channels");  }

                Channel = chan.channel;

                _callbacks = callback;
                _slot = chan.slot;

                _is_enabled = false;
                _is_mirror_state_IN = false;

                HandleEvent(chan);
                HandleEvent(mirro);
            }


            private readonly USBCallback _callbacks;
            private readonly byte _slot;

            private readonly object _lock = new();
            private bool _is_enabled;
            private bool _is_mirror_state_IN;


            private MsgHeader Header => new() { slot = _slot, channel = Channel };



            private struct MsgHeader
            {
                public byte slot;
                public int channel;
            }
            private struct ChannelEnabledPayload
            {
                public bool is_enabled;
            }
            private struct MirrorStatePayload
            {
                public bool is_state_IN;
            }
            private static byte[] RequestChannelEnabled(MsgHeader header)
            {
                byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + header.slot);
                var rt = new byte[6]
                { 0, 0, (byte)header.channel, 0, SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOD_REQ_CHANENABLESTATE), 0, rt, 0, 2);
                return rt;
            }
            private static byte[] Set(MsgHeader header, ChannelEnabledPayload payload)
            {
                byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + header.slot);
                var rt = new byte[6]
                { 0, 0, (byte)header.channel, (byte)(payload.is_enabled ? 1 : 0), SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOD_SET_CHANENABLESTATE), 0, rt, 0, 2);
                return rt;
            }

            private static byte[] RequestMirrorState(MsgHeader header)
            {
                byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + header.slot);
                var rt = new byte[6]
                { 0, 0, (byte)header.channel, 0, SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_REQ_MIRROR_STATE), 0, rt, 0, 2);
                return rt;
            }
            private static byte[] Set(MsgHeader header, MirrorStatePayload payload)
            {
                byte SLOT_ADDRESS = (byte)(Thorlabs.APT.Address.SLOT_1 + header.slot);
                var rt = new byte[6]
                { 0, 0, (byte)header.channel, (byte)(payload.is_state_IN ? 1 : 0), SLOT_ADDRESS, Thorlabs.APT.Address.HOST };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_SET_MIRROR_STATE), 0, rt, 0, 2);
                return rt;
            }
        }
        private struct SioBuilderCacheEntry
        {
            public ChannelEnableEventArgs? ChannelEnable;
            public MirrorStateEventArgs? MirrorState;

            public void Add(ChannelEnableEventArgs args)
            {  ChannelEnable = args; }

            public void Add(MirrorStateEventArgs args)
            {  MirrorState = args; }

            public readonly bool IsReady()
            { return ChannelEnable.HasValue && MirrorState.HasValue; }

            public readonly SioDriver Build(USBCallback callback)
            {
                return new SioDriver(callback, ChannelEnable.Value, MirrorState.Value);
            }
        }

        private class ShutterDriver : ReactiveObject, IShutterDriver
        {
            public int Slot
            { get; }
            public int Channel
            { get; }

            public bool IsEnabled
            {
                get { lock (_lock) { return _isEnabled; } }
                private set => this.ChangeWithLock(ref _isEnabled, value);
            }
            public IShutterDriver.States State
            {
                get { lock (_lock) { return _state; }}
                private set => this.ChangeWithLock(ref _state, value);
            }
            public IShutterDriver.TriggerModes ExternalTriggerMode
            {
                get { lock (_lock) { return _externalTriggerMode; }}
                private set => this.ChangeWithLock(ref _externalTriggerMode, value);
            }
            public IShutterDriver.Types Type
            {
                get { lock (_lock) { return _type; }}
                private set => this.ChangeWithLock(ref _type, value);
            }
            public IShutterDriver.States StateAtPowerUp
            {
                get { lock (_lock) { return _stateAtPowerUp; }}
                private set => this.ChangeWithLock(ref _stateAtPowerUp, value);
            }
            public double PulseDutyCyclePercentage
            {
                get { lock (_lock) { return _pulseDutyCyclePercentage; }}
                private set => this.ChangeWithLock(ref _pulseDutyCyclePercentage, value);
            }
            public double HoldDutyCyclePercentage
            {
                get { lock (_lock) { return _holdDutyCyclePercentage; }}
                private set => this.ChangeWithLock(ref _holdDutyCyclePercentage, value);
            }
            public TimeSpan PulseWidth
            {
                get { lock (_lock) { return _pulseWidth; }}
                private set => this.ChangeWithLock(ref _pulseWidth, value);
            }

            public ShutterDriver(USBCallback callbacks,
                ChannelEnableEventArgs chanEnable,
                SolenoidStateEventArgs solState,
                ShutterParamsEventArgs shutterParams)
            {
                _callbacks = callbacks;

                Slot = chanEnable.slot;
                Channel = chanEnable.channel;

                _isEnabled = chanEnable.is_enabled;
                _state = solState.solenoid_on ? IShutterDriver.States.OPEN : IShutterDriver.States.CLOSED;
                _externalTriggerMode = shutterParams.external_trigger_mode;
                _type = shutterParams.type;
                _stateAtPowerUp = shutterParams.power_up_state;
                _pulseDutyCyclePercentage = shutterParams.duty_cycle_percentage_pulse;
                _holdDutyCyclePercentage = shutterParams.duty_cycle_percentage_hold;
                _pulseWidth = shutterParams.pulse_width;
            }

            private static Task WriteUsbAsync(USBCallback callbacks, byte[] msg, CancellationToken token) => Task.Run(() =>
            {
                callbacks.WriteUSB(msg, msg.Length);
            }, token);

            public static void DispatchStateRequest(byte slot, int channel, USBCallback callbacks, CancellationToken token)
            {
                byte[] toSend = ChannelEnableEventArgs.SerializeRequest(slot, channel);
                WriteUsbAsync(callbacks, toSend, token).Wait(token);
                token.ThrowIfCancellationRequested();
                toSend = SolenoidStateEventArgs.SerializeRequest(slot, channel);
                WriteUsbAsync(callbacks, toSend, token).Wait(token);
                token.ThrowIfCancellationRequested();
            }
            public static void DispatchParameterRequest(byte slot, int channel, USBCallback callbacks, CancellationToken token)
            {
                byte[] toSend = ShutterParamsEventArgs.SerializeRequest(slot, channel);
                WriteUsbAsync(callbacks, toSend, token).Wait(token);
                token.ThrowIfCancellationRequested();
            }

            public void RequestState(CancellationToken token)
            {
                try
                {
                    DispatchStateRequest((byte)Slot, Channel, _callbacks, token);
                }
                catch
                { }
            }
            public void RequestParameters(CancellationToken token)
            {
                try
                {
                    DispatchParameterRequest((byte)Slot, Channel, _callbacks, token);
                }
                catch
                { }
            }

            public Task ConfigureBidirectionalPulseHold(double pulseDutyCyclePercentage, TimeSpan pulseWidth,
                double holdDutyCyclePercentage, IShutterDriver.States stateAtPowerUp, CancellationToken token) => Task.Run(() =>
            {
                byte[] toSend = new ShutterParamsEventArgs()
                {
                    slot = (byte)Slot,
                    channel = Channel,
                    power_up_state = stateAtPowerUp,
                    type = IShutterDriver.Types.BIDIRECTIONAL_PULSE_HOLD,
                    external_trigger_mode = ExternalTriggerMode,
                    duty_cycle_percentage_hold = holdDutyCyclePercentage,
                    duty_cycle_percentage_pulse = pulseDutyCyclePercentage,
                    pulse_width = pulseWidth,
                }.SerializeSet();

                _callbacks.WriteUSB(toSend, toSend.Length);
            }, token);

            public Task ConfigureNoDriver(CancellationToken token) => Task.Run(() =>
            {
                byte[] toSend = new ShutterParamsEventArgs()
                {
                    slot = (byte)Slot,
                    channel = Channel,
                    power_up_state = IShutterDriver.States.UNKNOWN,
                    type = IShutterDriver.Types.DISABLED,
                    external_trigger_mode = IShutterDriver.TriggerModes.DISABLED,
                    duty_cycle_percentage_hold = 0,
                    duty_cycle_percentage_pulse = 0,
                    pulse_width = TimeSpan.Zero,
                }.SerializeSet();

                _callbacks.WriteUSB(toSend, toSend.Length);
            }, token);

            public Task ConfigurePulsed(double pulseDutyCyclePercentage, TimeSpan pulseWidth,
                IShutterDriver.States stateAtPowerUp, CancellationToken token) => Task.Run(() =>
            {
                byte[] toSend = new ShutterParamsEventArgs()
                {
                    slot = (byte)Slot,
                    channel = Channel,
                    power_up_state = stateAtPowerUp,
                    type = IShutterDriver.Types.PULSED,
                    external_trigger_mode = ExternalTriggerMode,
                    duty_cycle_percentage_hold = 0,
                    duty_cycle_percentage_pulse = pulseDutyCyclePercentage,
                    pulse_width = pulseWidth,
                }.SerializeSet();

                _callbacks.WriteUSB(toSend, toSend.Length);
            }, token);

            public Task ConfigureUnidirectionalPulseHold(double pulseDutyCyclePercentage, TimeSpan pulseWidth,
                double holdDutyCyclePercentage, IShutterDriver.States stateAtPowerUp, CancellationToken token) => Task.Run(() =>
            {
                byte[] toSend = new ShutterParamsEventArgs()
                {
                    slot = (byte)Slot,
                    channel = Channel,
                    power_up_state = stateAtPowerUp,
                    type = IShutterDriver.Types.UNIDIRECTIONAL_PULSE_HOLD,
                    external_trigger_mode = ExternalTriggerMode,
                    duty_cycle_percentage_hold = holdDutyCyclePercentage,
                    duty_cycle_percentage_pulse = pulseDutyCyclePercentage,
                    pulse_width = pulseWidth,
                }.SerializeSet();

                _callbacks.WriteUSB(toSend, toSend.Length);
            }, token);

            public double DutyCycleToVoltage(double percentage)
            {
                return 24d * percentage / 100;
            }

            public Task SetEnableAsync(bool enable, CancellationToken token) => Task.Run(() =>
            {
                byte[] toSend = new ChannelEnableEventArgs()
                {
                    slot = (byte)Slot,
                    channel = Channel,
                    is_enabled = enable,
                }.SerializeSet();

                _callbacks.WriteUSB(toSend, toSend.Length);
            }, token);

            public Task SetExternalTriggerModeAsync(IShutterDriver.TriggerModes mode, CancellationToken token) => Task.Run(() =>
            {
                byte[] toSend;
                lock (_lock)
                {
                    toSend = new ShutterParamsEventArgs()
                    {
                        slot = (byte)Slot,
                        channel = Channel,
                        power_up_state = _stateAtPowerUp,
                        type = _type,
                        external_trigger_mode = mode,
                        duty_cycle_percentage_hold = _holdDutyCyclePercentage,
                        duty_cycle_percentage_pulse = _pulseDutyCyclePercentage,
                        pulse_width = _pulseWidth,
                    }.SerializeSet();
                }

                _callbacks.WriteUSB(toSend, toSend.Length);
            }, token);

            public Task SetStateAsync(IShutterDriver.States state, CancellationToken token) => Task.Run(() =>
            {
                byte[] toSend = new SolenoidStateEventArgs()
                {
                    slot = (byte)Slot,
                    channel = Channel,
                    solenoid_on = state == IShutterDriver.States.OPEN,
                }.SerializeSet();

                _callbacks.WriteUSB(toSend, toSend.Length);
            }, token);

            public double VoltageToDutyCycle(double voltage)
            {
                return voltage / 24d * 100;
            }


            public void HandleEvent(ChannelEnableEventArgs args)
            {
                IsEnabled = args.is_enabled;
            }
            public void HandleEvent(SolenoidStateEventArgs args)
            {
                State = args.solenoid_on ? IShutterDriver.States.OPEN : IShutterDriver.States.CLOSED;
            }
            public void HandleEvent(ShutterParamsEventArgs args)
            {
                ExternalTriggerMode = args.external_trigger_mode;
                Type = args.type;
                StateAtPowerUp = args.power_up_state;
                PulseDutyCyclePercentage = args.duty_cycle_percentage_pulse;
                HoldDutyCyclePercentage = args.duty_cycle_percentage_hold;
                PulseWidth = args.pulse_width;
            }

            private readonly USBCallback _callbacks;

            private readonly object _lock = new();
            private bool _isEnabled = false;
            private IShutterDriver.States _state = IShutterDriver.States.UNKNOWN;
            private IShutterDriver.TriggerModes _externalTriggerMode = IShutterDriver.TriggerModes.DISABLED;
            private IShutterDriver.Types _type = IShutterDriver.Types.DISABLED;
            private IShutterDriver.States _stateAtPowerUp = IShutterDriver.States.UNKNOWN;
            private double _pulseDutyCyclePercentage = 0;
            private double _holdDutyCyclePercentage = 0;
            private TimeSpan _pulseWidth = TimeSpan.Zero;


            private bool ChangeWithLock<T> (ref T obj, T value, [CallerMemberName] string propertyName = null)
            {
                bool changed;
                lock (_lock)
                {
                    changed = !EqualityComparer<T>.Default.Equals(obj, value);
                    obj = value;
                }
                if (changed)
                {
                    this.RaisePropertyChanged(propertyName);
                }

                return changed;
            }
        }
        private struct ShutterBuilderCacheEntry
        {
            public ChannelEnableEventArgs? ChannelEnable;
            public SolenoidStateEventArgs? SolenoidState;
            public ShutterParamsEventArgs? Params;

            public void Add(ChannelEnableEventArgs args)
            { ChannelEnable = args; }

            public void Add(SolenoidStateEventArgs args)
            { SolenoidState = args; }

            public void Add(ShutterParamsEventArgs args)
            { Params = args; }

            public readonly bool IsReady()
            {
                return ChannelEnable.HasValue
                  && SolenoidState.HasValue
                  && Params.HasValue;
            }

            public readonly ShutterDriver Build(USBCallback callback)
            {
                return new ShutterDriver(callback, ChannelEnable.Value,
                                         SolenoidState.Value, Params.Value);
            }
        }
  
    }

}
