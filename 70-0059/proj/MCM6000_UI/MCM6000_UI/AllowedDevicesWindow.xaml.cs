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
using System.Windows.Shapes;

namespace MCM6000_UI
{
    /// <summary>
    /// Interaction logic for AllowedDevicesWindow.xaml
    /// </summary>
    public partial class AllowedDevicesWindow : Window
    {
        public AllowedDevicesWindow(int slot_num, List<AllowedDevices.TableEntry> entries)
        {
            InitializeComponent();
            Title = "Allowed devices for slot " + slot_num.ToString() + ".";

            Content = new AllowedDevices(entries);
        }
    }
}
