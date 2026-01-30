using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.IO;
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

namespace MCM6000_UI
{
    /// <summary>
    /// Interaction logic for EFSFileDisplay.xaml
    /// </summary>
    public partial class EFSFileDisplay : UserControl
    {
        private readonly EFSFile _file;

        public byte Identifier { get { return _file.Identifier; } }
        public uint PageLength { get { return _file.PageLength; } }
        public bool Owned { get { return _file.Owned; } }
        public bool AER { get { return _file.Attributes.AptRead; } }
        public bool AEW { get { return _file.Attributes.AptWrite; } }
        public bool AED { get { return _file.Attributes.AptDelete; } }
        public bool AFR { get { return _file.Attributes.FirmwareRead; } }
        public bool AFW { get { return _file.Attributes.FirmwareWrite; } }
        public bool AFD { get { return _file.Attributes.FirmwareDelete; } }

        internal EFSFileDisplay(EFSFile file)
        {
            InitializeComponent();

            _file = file;

            DataContext = this;
        }

        private async void Button_Download_Clicked(object sender, RoutedEventArgs args)
        {
            SaveFileDialog sfd = new();
            sfd.Title = "Download \"" + Identifier.ToString() + "\" to...";
            sfd.DefaultExt = ".bin";
            sfd.Filter = "Binary Files|*.bin";
            bool? result = sfd.ShowDialog();
            if (result is null || !(bool)(result))
            {
                return;
            }

            string path = sfd.FileName;

            FileStream file;
            try
            {
                file = File.Open(path, FileMode.OpenOrCreate);
            }
            catch (Exception e)
            {
                MessageBox.Show("File error: " + e.Message, "File Error");
                return;
            }

            int LEN = (int)(_file.PageLength * _file.Parent.PageSize);
            byte[] data = new byte[LEN];
            try
            {
                data = await _file.ReadAsync(0, LEN);
            }
            catch (Exception e)
            {
                MessageBox.Show("Download error: " + e.Message, "Download Error");
                file.Close();
                return;
            }

            await file.WriteAsync(data, 0, data.Length);
            file.Close();
            
        }
        private void Button_Refresh_Clicked(object sender, RoutedEventArgs args)
        {
            Task.Run(() => _file.Parent.LookupFileAsync(_file.Extension, _file.Id));
        }
        private void Button_Delete_Clicked(object sender, RoutedEventArgs args)
        {
            MessageBoxResult result = MessageBox.Show("Are you sure you want to delete " + Identifier.ToString() + "?", "Confirm Delete", MessageBoxButton.YesNo);
            if (result != MessageBoxResult.Yes)
            {
                return;
            }
            Task.Run(() => _file.Parent.DeleteFileAsync(_file.Extension, _file.Id));
        }
    }
}
