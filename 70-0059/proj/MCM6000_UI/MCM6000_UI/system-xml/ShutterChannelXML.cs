using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.IO;
using System.Windows.Forms.DataVisualization.Charting;

namespace SystemXMLParser
{
    public class ShutterChannelXML : IPersistableConfigurable
    {
        private Dictionary<ushort, byte[]> _data = new();

        public int Slot { get; }

        public int Channel { get; }

        public MCM6000_UI.controls.flipper_shutter.ShutterParamsEventArgs? Config
        { get; private set; } = null;

        public ShutterChannelXML()
        { }

        public ShutterChannelXML(XmlElement element, int slot, int channel)
        {
            Slot = slot;
            Channel = channel;
            Setup(element);
        }

        private struct NodeQuery
        {
            public XmlNode PowerUpPosition = null;
            public XmlNode TriggerMode = null;
            public XmlNode Type = null;
            public XmlNode OnTime = null;
            public XmlNode PulseVoltage = null;
            public XmlNode HoldVoltage = null;
            public XmlNode OpenUsesNegativeVoltage = null;
            public XmlNode HoldoffTimeSeconds = null;

            public readonly bool Complete =>
                PowerUpPosition is not null &&
                TriggerMode is not null &&
                Type is not null &&
                OnTime is not null &&
                PulseVoltage is not null &&
                HoldVoltage is not null;


            public NodeQuery() { }
        }
        private NodeQuery GetParts(XmlElement element)
        {
            return new NodeQuery()
            {
                PowerUpPosition = element.SelectSingleNode("PowerUpPosition"),
                TriggerMode = element.SelectSingleNode("TriggerMode"),
                Type = element.SelectSingleNode("Type"),
                OnTime = element.SelectSingleNode("OnTimeSeconds"),
                PulseVoltage = element.SelectSingleNode("PulseVoltage"),
                HoldVoltage = element.SelectSingleNode("HoldVoltage"),
                OpenUsesNegativeVoltage = element.SelectSingleNode("OpenUsesNegativeVoltage"),
                HoldoffTimeSeconds = element.SelectSingleNode("HoldoffTimeSeconds"),
            };
        }

        private void Setup(XmlElement element)
        {
            var query = GetParts(element);

            if (query.Complete)
            {
                bool success = true;
                var cfg = new MCM6000_UI.controls.flipper_shutter.ShutterParamsEventArgs()
                {
                    slot = (byte)(Slot - 1),
                    channel = Channel,
                };

                string StringValue(XmlNode node, string attr = "value") =>
                    node.Attributes.GetNamedItem(attr).Value;

                bool TryFloatingValue(out double value, XmlNode node, string attr = "value") =>
                    double.TryParse(StringValue(node, attr), out value);

                bool BoolValue(XmlNode node, string attr = "value")
                {
                    return node.Attributes.GetNamedItem(attr).Value.ToLower() switch
                    {
                        "true" or "yes" or "on" or "1" => true,
                        "false" or "no" or "off" or "0" => false,
                        _ => throw new ArgumentException(),
                    };
                }
                
                switch(StringValue(query.PowerUpPosition))
                {
                    case "UNKNOWN":
                        cfg.power_up_state = MCM6000_UI.controls.drivers.IShutterDriver.States.UNKNOWN;
                        break;
                    case "OPEN":
                        cfg.power_up_state = MCM6000_UI.controls.drivers.IShutterDriver.States.OPEN;
                        break;
                    case "CLOSED":
                        cfg.power_up_state = MCM6000_UI.controls.drivers.IShutterDriver.States.CLOSED;
                        break;
                    default:
                        success = false;
                        break;
                }

                switch(StringValue(query.TriggerMode))
                {
                    case "DISABLED":
                        cfg.external_trigger_mode = MCM6000_UI.controls.drivers.IShutterDriver.TriggerModes.DISABLED;
                        break;
                    case "ENABLED":
                        cfg.external_trigger_mode = MCM6000_UI.controls.drivers.IShutterDriver.TriggerModes.ENABLED;
                        break;
                    case "INVERTED":
                        cfg.external_trigger_mode = MCM6000_UI.controls.drivers.IShutterDriver.TriggerModes.INVERTED;
                        break;
                    default:
                        success = false;
                        break;
                }

                switch(StringValue(query.Type))
                {
                    case "PULSED":
                        cfg.type = MCM6000_UI.controls.drivers.IShutterDriver.Types.PULSED;
                        break;
                    case "PULSE HOLD":
                    case "BIDIRECTIONAL PULSE HOLD":
                        cfg.type = MCM6000_UI.controls.drivers.IShutterDriver.Types.BIDIRECTIONAL_PULSE_HOLD;
                        break;
                    case "NO RETURN":
                    case "UNIDIRECTION PULSE HOLD":
                    case "NO REVERSE":
                        cfg.type = MCM6000_UI.controls.drivers.IShutterDriver.Types.UNIDIRECTIONAL_PULSE_HOLD;
                        break;
                    default:
                        success = false;
                        break;
                }

                if (TryFloatingValue(out double onTimeSeconds, query.OnTime))
                {
                    cfg.pulse_width = TimeSpan.FromSeconds(onTimeSeconds);
                    success = success && onTimeSeconds > 0;
                }
                else
                {
                    success = false;
                }

                const double MIN_VOLTAGE = 0;
                const double MAX_VOLTAGE = 24;

                static bool IsVoltageInRange(double voltage) =>
                    MIN_VOLTAGE <= voltage && voltage <= MAX_VOLTAGE;
                static double VoltageToPercentageDuty(double voltage) =>
                    100 * (voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE);

                if (TryFloatingValue(out double pulseVoltage, query.PulseVoltage))
                {
                    cfg.duty_cycle_percentage_pulse = VoltageToPercentageDuty(pulseVoltage);
                    success = success && IsVoltageInRange(pulseVoltage);
                }
                else
                {
                    success = false;
                }

                if (TryFloatingValue(out double holdVoltage, query.HoldVoltage))
                {
                    cfg.duty_cycle_percentage_hold = VoltageToPercentageDuty(holdVoltage);
                    success = success && IsVoltageInRange(holdVoltage);
                }
                else
                {
                    success = false;
                }

                // Optional field
                if (query.OpenUsesNegativeVoltage is not null)
                {
                    cfg.open_uses_negative_voltage = BoolValue(query.OpenUsesNegativeVoltage);
                } else
                {
                    cfg.open_uses_negative_voltage = false;
                }

                // Optional field
                if (query.HoldoffTimeSeconds is not null)
                {
                    if (TryFloatingValue(out double time, query.HoldoffTimeSeconds))
                    {
                        cfg.holdoff_time = TimeSpan.FromSeconds(time);
                    }
                    else
                    {
                        success = false;
                    }
                } else
                {
                    cfg.holdoff_time = TimeSpan.Zero;
                }


                if (success)
                {
                    Config = cfg;
                }
            }

            if (Config is not null)
            {
                byte[] msg = Config.Value.SerializeSet();

                _data[Thorlabs.APT.MGMSG_MCM_SET_SHUTTERPARAMS] = msg;
            }
        }

        public void Configure(ConfigurationParams args) => Configure(args, false);

        /// <summary>
        /// If save to eeprom is true, then all the commands will also be saved to EEPROM.
        /// </summary>
        public void Configure(ConfigurationParams args, bool save_to_eeprom)
        {
            foreach (KeyValuePair<ushort, byte[]> kvp in _data)
            {
                args.usb_send(kvp.Value);
            }

            // Save to EEPROM if flagged.
            if (save_to_eeprom)
            {
                byte[] command = { 0, 0, 4, 0, (byte)(Thorlabs.APT.Address.SLOT_1 + (Slot - 1) | 0x80), Thorlabs.APT.Address.HOST, 0, 0, 0, 0 };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_SET_EEPROMPARAMS), 0, command, 0, 2);
                Array.Copy(BitConverter.GetBytes(Channel), 0, command, 6, 2);

                foreach (KeyValuePair<ushort, byte[]> kvp in _data)
                {
                    Array.Copy(BitConverter.GetBytes(kvp.Key), 0, command, 8, 2);
                    args.usb_send(command);
                }
            }
        }
    }
}
