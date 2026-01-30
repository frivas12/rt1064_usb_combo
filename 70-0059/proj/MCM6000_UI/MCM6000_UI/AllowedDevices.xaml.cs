using Microsoft.VisualBasic.FileIO;
using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
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
using System.Windows.Shapes;

namespace MCM6000_UI
{
    public class AllowedDevicesWindow : Window
    {
        private AllowedDevices m_ad;

        public AllowedDevicesWindow(int slot_num, List<AllowedDevices.TableEntry> entires)
        {
            Title = "Allowed devices for slot " + slot_num.ToString() + ".";
            m_ad = new AllowedDevices(entires);

            Content = m_ad;

            SizeToContent = SizeToContent.WidthAndHeight;

            Closing += OnClosing;
            m_ad.Committed += delegate ()
            {
                entires.Clear();
                foreach (AllowedDevices.TableEntry entry in m_ad.Entries)
                {
                    entires.Add(entry);
                }
                ForceClose();
            };
        }

        private void OnClosing(object sender, CancelEventArgs e)
        {
            DialogResult = m_ad.Updated;
        }

        private void ForceClose()
        {
            Close();
        }
    }


    public sealed partial class AllowedDevices : Page
    {
        private bool m_updated = false;

        public ObservableCollection<TableEntry> Entries { get; set; }

        public bool Updated { get { return m_updated;} }

        public class TableEntry
        {
            public string SlotType { get; }
            public uint DeviceID { get;  }
            public bool ToRemove { get; set; }

            public TableEntry(string st, uint dt)
            {
                SlotType = st;
                DeviceID = dt;
            }
        }

        public AllowedDevices(List<TableEntry> entries)
        {
            InitializeComponent();

            foreach (CardTypes ct in Enum.GetValues(typeof(CardTypes)))
            {
                if (ct != CardTypes.NO_CARD_IN_SLOT && ct != CardTypes.CARD_IN_SLOT_NOT_PROGRAMMED)
                {
                    ComboBoxItem cbi = new ComboBoxItem();
                    cbi.Content = ct.ToString();
                    e_ad_slot_id.Items.Add(cbi);
                }
            }

            Entries = new ObservableCollection<TableEntry>(entries); // Reference copy.

            DataContext = this;
        }

        private void AddEntry(object sender, RoutedEventArgs e)
        {
            ushort dt;
            if ((e_ad_device_id.Text.Length > 2 && e_ad_device_id.Text.Substring(0, 2).ToLower() == "0x" && ushort.TryParse(e_ad_device_id.Text.Substring(2), System.Globalization.NumberStyles.HexNumber, System.Globalization.NumberFormatInfo.CurrentInfo, out dt))
                || ushort.TryParse(e_ad_device_id.Text, out dt))
            {
                try
                {
                    TableEntry entry = new TableEntry(
                        ((ComboBoxItem)e_ad_slot_id.SelectedItem).Content.ToString(),
                        dt
                    );

                    Entries.Add(entry);
                    e_ad_allowed.Items.Refresh();
                }
                catch
                {

                }
            }
        }

        private void RemoveEntries(object sender, RoutedEventArgs e)
        {
            for (int i = 0; i < Entries.Count;)
            {
                TableEntry ENTRY = Entries[i];
                if (ENTRY.ToRemove)
                {
                    Entries.RemoveAt(i);
                } else
                {
                    ++i;
                }
            }
        }

        private void Commit (object sender, RoutedEventArgs e)
        {
            m_updated = true;
            Committed?.Invoke();
        }

        private void LoadFromFile (object sender, RoutedEventArgs e)
        {
            OpenFileDialog FWDialog = new OpenFileDialog();
            FWDialog.Title = "Select .csv file(s) using this Dialog Box";
            FWDialog.Filter = "Allowed Devices File|*.csv";
            FWDialog.Multiselect = false;
            if (FWDialog.ShowDialog() == true)
            {
                using (TextFieldParser parser = new TextFieldParser(FWDialog.FileName))
                {
                    Entries.Clear();

                    parser.TextFieldType = FieldType.Delimited;
                    parser.SetDelimiters(",");
                    while (!parser.EndOfData)
                    {
                        string[] fields = parser.ReadFields();
                        if (fields.Length < 2)
                        {
                            throw new IndexOutOfRangeException();
                        }

                        TableEntry entry = new TableEntry(fields[0], uint.Parse(fields[1]));

                        Entries.Add(entry);
                    }
                }
            }
        }

        public event Action Committed;
    }
}
