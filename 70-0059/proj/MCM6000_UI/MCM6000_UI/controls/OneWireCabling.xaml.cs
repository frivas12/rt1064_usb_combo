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

namespace MCM6000_UI.controls
{
    /// <summary>
    /// OneWireCabling is a view for one-wire programming
    /// where the cable part number has a one-to-one relationship
    /// with the device identification.
    /// </summary>
    public partial class OneWireCabling : UserControl
    {
        public OneWireCabling()
        {
            InitializeComponent();
        }
    }
}
