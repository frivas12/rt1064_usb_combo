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
    /// Interaction logic for OneWireConfigurationV2.xaml
    /// </summary>
    public partial class OneWireConfigurationV2 : UserControl
    {
        public OneWireConfigurationV2()
        {
            InitializeComponent();

            e_tbx_sn.PreviewKeyDown += (s, a) =>
            {
                if (a.Key == Key.Enter
                 || a.Key == Key.Escape)
                {
                    ((Control)s).MoveFocus(new TraversalRequest(FocusNavigationDirection.Up));
                }
            };
        }
    }
}
