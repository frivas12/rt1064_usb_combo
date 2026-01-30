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
    /// Interaction logic for servo_view.xaml
    /// </summary>
    public partial class servo : Page
    {
        // BEGIN    UI Properties

        // END      UI Properties

        // BEGIN    Event Arguments

        public class OnStatusUpdateArgs
        {
            public byte Slot { get; }               // Zero-Indexed
        }


        // END      Event Arguments

        // BEGIN    Event Handlers

        public delegate void OnStatusUpdateHandler(object sender, OnStatusUpdateArgs args);

        // END      Event Handlers

        // BEGIN    Events

        public event OnStatusUpdateHandler OnStatusUpdate;

        // END      Events

        // BEGIN    Constructors
        public servo()
        {
            InitializeComponent();

            OnStatusUpdate += StatusUpdate;
        }
        // END      Constructors

        // BEGIN    Public Interface

        // END      Public Interface

        // BEGIN    Private Functions

        void StatusUpdate(object sender, OnStatusUpdateArgs args)
        {

        }
    }
}
