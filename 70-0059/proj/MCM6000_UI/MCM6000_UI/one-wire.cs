using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.Tracing;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Reactive.Subjects;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Xml;
using SystemXMLParser;

namespace MCM6000_UI
{
    public partial class MainWindow
    {
        private const string ONEWIRE_PACKAGE_PATH = "xml/system/packages/onewire.zip";
        private const string SUPPORTED_DEVICES_PATH = "SupportedDevices.csv";

        // dict: mode -> key -> full archive name
        private readonly Lazy<Dictionary<string, Dictionary<string, string>>> _supportedOnewireDevices;

        private readonly BehaviorSubject<builders.OneWireBuilderViewModel> _owBuilderVM = new(null);

        private int m_ow_slot_selected = 1;
        private DeviceSignature m_ow_selected_slot_signature = null;

        private Semaphore m_ow_response_ready = new Semaphore(0, 1);
        private byte[] m_ow_response_buffer = new byte[20]; /// \todo Make this value (a) make sense and (b) not a magic number.

        const string OW_SHOW_CUSTOM_PART_TEXT = "Enable Custom Part Number";
        const string OW_HIDE_CUSTOM_PART_TEXT = "Disable Custom Part Number";

        private void safe_set_content(ContentControl obj, string content)
        {
            if (ow_status.Dispatcher.CheckAccess())
            {
                obj.Content = content;
            }
            else
            {
                obj.Dispatcher.BeginInvoke(new Action(() => safe_set_content(obj, content)));
            }
        }

        private void ow_refresh_device_signature()
        {
            if (m_ow_selected_slot_signature != null)
            {
                safe_set_content(ow_unique_slot_type, m_ow_selected_slot_signature.SlotType.ToString());
                safe_set_content(ow_unique_device_type, "0x" + m_ow_selected_slot_signature.DeviceType.ToString("X4"));
            }
            else
            {
                safe_set_content(ow_unique_slot_type, "");
                safe_set_content(ow_unique_device_type, "");
            }
        }

        private void ow_refresh_page()
        {
            ow_refresh_device_signature();
        }

        private void ow_refresh_Clicked(object sender, RoutedEventArgs args) => ow_request_device();

        private void ow_request_device_callback(byte[] ext_data)
        {
            if (ext_data != null && ext_data.Length == 31 && ext_data[0] == m_ow_slot_selected - 1)
            {
                // Get Device ID
                ushort device_id = BitConverter.ToUInt16(ext_data, 2);

                // Get Serial Number
                ulong serial_num = BitConverter.ToUInt64(ext_data, 4);
                string sn_str = serial_num.ToString("X4");
                sn_str = "0x" + sn_str.PadLeft(12, '0'); ;

                ushort slot_id = BitConverter.ToUInt16(ext_data, 12);

                string product_number = Encoding.ASCII.GetString(ext_data.Skip(14).Take(16).ToArray());
                try
                {
                    CardTypes ct = (CardTypes)slot_id;
                    m_ow_selected_slot_signature = new DeviceSignature(ct, device_id);
                    product_number = "(" + product_number + ")";

                    ow_refresh_device_signature();
                    safe_set_content(ow_read_slot_type, ct.ToString());
                    safe_set_content(ow_read_device_type, "0x" + device_id.ToString("X4"));
                    safe_set_content(ow_read_device_name, product_number);
                    safe_set_content(ow_read_serial_number, sn_str);
                } catch
                {
                    // \todo Show some kind of error.
                }
            }
        }

        private void ow_request_device()
        {
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_REQ_DEVICE));
            byte[] bytesToSend = new byte[6] { command[0], command[1], (byte)(m_ow_slot_selected - 1), 0x00, MOTHERBOARD_ID, HOST_ID };

            usb_write("one-wire", bytesToSend, 6);
        }

        private byte[] OneWireFetchAndConfigure(builders.OneWireBuilderViewModel vm)
        {
            if (!vm.IsReady)
            { throw new InvalidOperationException("configuration not ready"); }
            if (!_supportedOnewireDevices.Value.TryGetValue(vm.ModeKey, out var devicesDict))
            { throw new InvalidOperationException($"mode key {vm.ModeKey} is invalid"); }
            if (!devicesDict.TryGetValue(vm.DeviceKey, out string archivePath))
            { throw new InvalidOperationException($"device key {vm.DeviceKey} is invalid"); }

            using MemoryStream stream = new();

            using (var fileStream = File.OpenRead(ONEWIRE_PACKAGE_PATH))
            {
                using var archive = new ZipArchive(fileStream, ZipArchiveMode.Read);
                var entry = archive.GetEntry(archivePath)
                    ?? throw new InvalidOperationException($"onewire package has no entry named \"{archivePath}\"");

                using var entryStream = entry.Open();
                entryStream.CopyTo(stream);
            }

            byte[] rt = new byte[stream.Length];
            Array.Copy(stream.GetBuffer(), rt, (int)stream.Length);

            vm.ConfigurePayload(rt, 0, rt.Length);
            return rt;
        }

        private void ow_init()
        {
            foreach (CardTypes ct in Enum.GetValues(typeof(CardTypes)))
            {
                if (ct != CardTypes.NO_CARD_IN_SLOT && ct != CardTypes.CARD_IN_SLOT_NOT_PROGRAMMED)
                {
                    ComboBoxItem cbi = new ComboBoxItem();
                    cbi.Content = ct.ToString();
                }
            }

            void Execute()
            {
                var vm = _owBuilderVM.Value;
                Debug.Assert(vm is not null);
                byte[] payload;
                try
                {
                    payload = OneWireFetchAndConfigure(vm);
                }
                catch (InvalidOperationException ex)
                {
                    safe_set_content(ow_status, ex.Message);
                    return;
                }

                SystemXMLParser.ConfigurationParams args = new()
                {
                    DEFAULT_TIMEOUT = 2000,
                    usb_send = (b) => { usb_write("one-wire",b, b.Length); Thread.Sleep(15); },
                    usb_send_fragment = (a, b) => { usb_write("one-wire",a, b); Thread.Sleep(15); },
                    usb_response = () => { m_ow_response_ready.WaitOne(); return m_ow_response_buffer; },
                    usb_timeout = (t) => { return m_ow_response_ready.WaitOne(t) ? m_ow_response_buffer : null; },
                    restart_board = () => { Restart_processor(false); },
                };


                string err = Utils.OneWireProgrammingRoutine(args, (byte)(m_ow_slot_selected - 1), payload);
                if (err is not null)
                {
                    Dispatcher.Invoke(() =>
                    {
                        MessageBox.Show("One-Wire Response:  " + err);
                    });
                    return;
                }
            }

            e_btn_ow_execute.Command = ReactiveCommand.CreateFromTask(() =>
                Task.Run(Execute),
                _owBuilderVM.SelectMany(x => x?.WhenAnyValue(y => y.IsReady) ?? Observable.Return(false))
            );

            
            static object PresenterContentSwitch(object vm)
            {
                return vm switch
                {
                    builders.OneWireV2BuilderViewModel => new controls.OneWireConfigurationV2()
                    {
                        DataContext = vm,
                    },
                    builders.OneToOneViewModel => new controls.OneWireCabling()
                    {
                        DataContext = vm,
                    },
                    builders.OneWireSystemViewModel => new controls.OneWireSystem()
                    {
                        DataContext = vm,
                    },
                    _ => null,
                };
                ;
            }

            // Not disposed b/c I don't know where to put it.
            _owBuilderVM.Do(x =>
            {
                e_cpr_one_wire_config.Content = PresenterContentSwitch(x);
            })
                                     .Subscribe();
            void ow_device_select_SelectionChanged(object sender, SelectionChangedEventArgs e)
            {
                m_ow_selected_slot_signature = null;
                m_ow_slot_selected = ((ComboBox)sender).SelectedIndex;
                m_ow_slot_selected = m_ow_slot_selected + ((m_ow_slot_selected >= 0) ? 1 : 0);
                ow_refresh_page();

                Task.Factory.StartNew(new Action(ow_request_device));
            }
            ow_device_select.SelectionChanged += ow_device_select_SelectionChanged;
        }

        private builders.OneWireV2BuilderViewModel CreateOneWireV2ViewModel(IEnumerable<string> partTypes)
        {
            const int SERIAL_NUMBER_LENGTH = 6;

            var validCharacters = new Regex("[\\d\\w]+");

            var rt = new builders.OneWireV2BuilderViewModel()
            {
                SerialNumberLength = SERIAL_NUMBER_LENGTH,
                SerialNumberValidator = str => str.Length == SERIAL_NUMBER_LENGTH
                                            && validCharacters.IsMatch(str),
                PartTypes = partTypes,
            };

            return rt;
        }
        private builders.OneToOneViewModel CreateOneToOneViewModel(IEnumerable<string> partTypes)
        {
            var rt = new builders.OneToOneViewModel()
            {
                PartTypes = partTypes,
            };

            return rt;
        }
        private builders.OneWireSystemViewModel CreateSystemViewModel( Dictionary<string, IEnumerable<string>> systemToAxes)
        {
            var rt = new builders.OneWireSystemViewModel()
            {
                SystemAndAxes = systemToAxes,
            };

            return rt;
        }


        private void OneWireVersionSelectionChanged(object sender, SelectionChangedEventArgs args)
        {
            builders.OneWireBuilderViewModel next;

            builders.OneWireBuilderViewModel GetVersionViewModel(string mode)
            {
                builders.OneWireBuilderViewModel CreateVersion2(Dictionary<string, string> partNameToBasePath)
                {
                    var vm = CreateOneWireV2ViewModel(partNameToBasePath.Keys);
                    return vm;
                }

                builders.OneWireBuilderViewModel CreateSystem(Dictionary<string, string> keyToXmlFile)
                {
                    // The key is encoded as <SYSTEM>/<AXIS>.
                    Dictionary<string, Dictionary<string, string>> systemToAxisToBaseFile = [];
                    Dictionary<string, IEnumerable<string>> systemToAxes = [];
                    foreach (var encodedKey in keyToXmlFile.Keys)
                    {
                        string[] segments = encodedKey.Split('/');
                        if (segments.Length != 2)
                        {
                            throw new FileFormatException($"key \"{encodedKey}\" does not contain a depth-2 path (actual: {segments.Length})");
                        }
                        var systemKey = segments[0];
                        var axisKey = segments[1];

                        if (!systemToAxisToBaseFile.TryGetValue(systemKey, out var systemDict))
                        {
                            systemDict = [];
                            systemToAxisToBaseFile[systemKey] = systemDict;
                            systemToAxes[systemKey] = new List<string>();
                        }
                        var sysList = (List<string>)systemToAxes[systemKey];

                        if (systemToAxisToBaseFile.ContainsKey(axisKey))
                        {
                            throw new FileFormatException($"key \"{encodedKey}\" is duplicated");
                        }

                        systemDict[axisKey] = keyToXmlFile[encodedKey];
                        sysList.Add(axisKey);
                    }

                    var vm = CreateSystemViewModel(systemToAxes);

                    return vm;
                }

                builders.OneWireBuilderViewModel CreateOneToOne(Dictionary<string, string> partNameToBasePath)
                {
                    var vm = CreateOneToOneViewModel(partNameToBasePath.Keys);
                    return vm;
                }

                var parts = _supportedOnewireDevices.Value;
                if (!parts.TryGetValue(mode, out var typeDict))
                {
                    typeDict = [];
                }
                return mode switch
                {
                    "v2" => CreateVersion2(typeDict),
                    "1:1" => CreateOneToOne(typeDict),
                    "system" => CreateSystem(typeDict),
                    _ => null,
                };
            }

            if (args.AddedItems.Count == 1 && args.AddedItems[0] is ComboBoxItem item)
            {
                switch (item.Content)
                {
                    case string version:
                        next  = GetVersionViewModel(version);
                        break;

                    default:
                        next = null;
                        break;
                }
            }
            else
            {
                next = null;
            }

            _owBuilderVM.OnNext(next);
        }

        private Dictionary<string, Dictionary<string, string>> GetSupportedDevices()
        {
            using var stream = File.OpenText(SUPPORTED_DEVICES_PATH);
            Dictionary<string, Dictionary<string, string>> rt = [];

            static string RemoveStartingQuotes(string str)
            {
                if (str.First() != '"'
                 || str.Last() != '"')
                    return str;

                return str.Substring(1, str.Length - 2)
                          .Replace("\\\"", "\"");
            }

            for (string line = stream.ReadLine();
                line is not null;
                line = stream.ReadLine())
            {
                var segments = line.Split(',');
                if (segments.Length != 3)
                    throw new FormatException("csv did not have 3 segments");
                var type = segments[0];
                var part = segments[1];
                var path = segments[2];

                if (!rt.TryGetValue(type, out var typeDict))
                {
                    typeDict = [];
                    rt[type] = typeDict;
                }
                typeDict[part] = RemoveStartingQuotes(path);
            }

            return rt;
        }
    }
}
