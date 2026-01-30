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
using System.IO.Ports;
using System.IO;
using System.Timers;
using Microsoft.Win32;
using System.Threading;
using System.ComponentModel;
using System.Text.RegularExpressions;
using System.Collections.Concurrent;
using static System.Net.WebRequestMethods;
using Microsoft.Office.Interop.Excel;

namespace MCM6000_UI
{
    public class EFSAttributes
    {
        private readonly byte _attributes;

        public bool AptRead {
            get { return (_attributes & 0x01) > 0; }
        }
        public bool AptWrite {
            get { return (_attributes & 0x02) > 0; }
        }
        public bool AptDelete {
            get { return (_attributes & 0x04) > 0; }
        }

        public bool FirmwareRead {
            get { return (_attributes & 0x08) > 0; }
        }
        public bool FirmwareWrite {
            get { return (_attributes & 0x10) > 0; }
        }
        public bool FirmwareDelete {
            get { return (_attributes & 0x20) > 0; }
        }

        public EFSAttributes(byte attributes)
        {
            _attributes = attributes;
        }

        public EFSAttributes(
            bool apt_read = true,
            bool apt_write = true,
            bool apt_delete = true,
            bool firmware_read = false,
            bool firmware_write = false,
            bool firmware_delete = false
        ) {
            byte attr = (byte)(
                (apt_read ? 0x01 : 0 ) |
                (apt_write ? 0x02 : 0) |
                (apt_delete ? 0x04 : 0) |
                (firmware_read ? 0x08 : 0) |
                (firmware_write ? 0x10 : 0) |
                (firmware_delete ? 0x20 : 0)
            );
            _attributes = attr;
        }

        public static explicit operator byte(EFSAttributes attr) => attr._attributes;
    }

    public class EFSFile
    {
        private readonly byte _identifier;

        public EFSDriver Parent { get; }

        public byte Identifier { get {return _identifier;}}

        public uint Extension
        {
            get { return (uint)(_identifier << 5); }
        }
        public uint Id 
        {
            get { return (uint)(_identifier & 0x1F); }
        }
        public uint PageLength { get; }
        public bool Owned { get; }
        public EFSAttributes Attributes { get; }

        public event EventHandler<byte[]> DataAvailable;

        public async Task<byte[]> ReadAsync(uint address, int bytes_to_read)
        {
            byte[] bytes_to_send = new byte[6 + 7];
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_EFS_REQ_FILEDATA), 0, bytes_to_send, 0, 2);
            bytes_to_send[2] = 7;
            bytes_to_send[3] = 0;
            bytes_to_send[4] = (byte)((byte)Thorlabs.APT.Address.MOTHERBOARD | 0x80);
            bytes_to_send[5] = (byte)(Thorlabs.APT.Address.HOST);
            bytes_to_send[6] = _identifier;

            SemaphoreSlim queue_length = new(0);
            ConcurrentQueue<byte[]> data_received = new();
            List<byte[]> data_packets = new();
            uint array_len = 0;

            EventHandler<byte[]> f = (sender, data) => {
                if (sender == Parent && data[0] == _identifier)
                {
                    data_received.Enqueue(data);
                    queue_length.Release();
                }
            };

            DataAvailable += f;

            while (bytes_to_read > 0)
            {
                Array.Copy(BitConverter.GetBytes(address), 0, bytes_to_send, 7, 4);
                Array.Copy(BitConverter.GetBytes(bytes_to_read), 0, bytes_to_send, 11, 2);
                Parent.Write(bytes_to_send, 0, bytes_to_send.Length);

                await queue_length.WaitAsync();

                byte[] data;
                while (!data_received.TryDequeue(out data))
                {
                    Thread.Yield();
                }

                int DATA_AVAILABLE = data.Length - 5;
                uint ADDRESS = BitConverter.ToUInt32(data, 1);
                if (DATA_AVAILABLE <= 0)
                {
                    // Exit 
                    break;
                }
                if (ADDRESS != address)
                {
                    throw new Exception("address mismatch");
                }

                data_packets.Add(data);

                array_len += (uint)DATA_AVAILABLE;
                address += (uint)DATA_AVAILABLE;
                bytes_to_read -= DATA_AVAILABLE;                
            }

            DataAvailable -= f;


            
            byte[] rt = new byte[array_len];
            int offset = 0;
            foreach (var item in data_packets)
            {
                int DATA_AVAILABLE = item.Length - 5;
                Array.Copy(item, 5, rt, offset, DATA_AVAILABLE);
                offset += DATA_AVAILABLE;
            }

            return rt;

        }
        public void Write(uint address, byte[] bytes_to_write)
        {
            uint BYTES_IN_PACKET = (bytes_to_write.Length > (Parent.ProgrammingPayloadLimit - 5)) ? 
                (Parent.ProgrammingPayloadLimit - 5) : (uint)bytes_to_write.Length;
            byte[] bytes_to_send = new byte[6 + 5 + BYTES_IN_PACKET];
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_EFS_SET_FILEDATA), 0, bytes_to_send, 0, 2);
            // Index 2 and 3 will be set dynamically.
            bytes_to_send[4] = (byte)((byte)Thorlabs.APT.Address.MOTHERBOARD | 0x80);
            bytes_to_send[5] = (byte)(Thorlabs.APT.Address.HOST);
            bytes_to_send[6] = _identifier;
            // Index 7, 8, 9, and 10 will be set dynamically

            int index = 0;
            while (index < bytes_to_write.Length)
            {
                // Populate data
                uint BYTES_TO_WRITE = ((bytes_to_write.Length - index) > BYTES_IN_PACKET) ? BYTES_IN_PACKET : (uint)(bytes_to_write.Length - index);
                Array.Copy(BitConverter.GetBytes(5 + BYTES_TO_WRITE), 0, bytes_to_send, 2, 2);
                Array.Copy(BitConverter.GetBytes(address), 0, bytes_to_send, 7, 4);
                Array.Copy(bytes_to_write, index, bytes_to_send, 11, BYTES_TO_WRITE);

                Parent.Write(bytes_to_send, 0, (int)(6 + 5 + BYTES_TO_WRITE));
                index += (int)BYTES_TO_WRITE;
                address += BYTES_TO_WRITE;
            }
        }
        

        internal EFSFile(
            EFSDriver driver,
            byte identifier,
            bool owned,
            byte attributes,
            uint page_length
        )
        {
            Parent = driver;
            _identifier =  identifier;
            Owned = owned;
            PageLength = page_length;
            Attributes = new EFSAttributes(attributes);
        }

        internal void RouteEvent(object sender, byte[] data)
        {
            DataAvailable?.Invoke(sender, data);
        }
    }

    public sealed class EFSDriver
    {
        private List<EFSFile> _files = new();

        public uint ProgrammingPayloadLimit { get; private set; }

        public string Identifier { get; private set; }
        public uint FileVersion { get; private set; }
        public uint PageSize { get; private set; }
        public uint TotalPages { get; private set; }
        public uint MaximumFiles { get; private set; }
        public uint FilesRemaining { get; private set; }
        public uint PagesRemaining { get; private set; }

        public IEnumerable<EFSFile> Files
        {
            get {
                EFSFile[] rt = new EFSFile[_files.Count];
                _files.CopyTo(rt);
                return rt;
            }
        }

        public void RequestHeaderInfo()
        {
            byte[] bytes_to_send = new byte[6];
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_EFS_REQ_HWINFO), 0, bytes_to_send, 0, 2);
            bytes_to_send[2] = 0;
            bytes_to_send[3] = 0;
            bytes_to_send[4] = (byte)(Thorlabs.APT.Address.MOTHERBOARD);
            bytes_to_send[5] = (byte)(Thorlabs.APT.Address.HOST);

            Write(bytes_to_send, 0, 6);
        }

        public async Task LookupAllFilesAsync()
        {
            for (int i = 0; i < 256; ++i)
            {
                await LookupFileAsync((byte)i);
            }
        }

        public async Task<EFSFile?> LookupFileAsync(uint extension, uint id) => await LookupFileAsync((byte)((extension << 5) | (id & 0x1F)));
        private async Task<EFSFile?> LookupFileAsync(byte identifier)
        {
            SemaphoreSlim data_arrived = new(0);
            byte[] data_available = Array.Empty<byte>();

            EventHandler<byte[]> f = (sender, data) => {
                byte IDENTIFIER = data[0];
                if (IDENTIFIER == identifier)
                {
                    data_available = data;
                    data_arrived.Release();
                }
            };

            DataReceived += f;

            byte[] bytes_to_send = new byte[7];
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_EFS_REQ_FILEINFO), 0, bytes_to_send, 0, 2);
            bytes_to_send[2] = 1;
            bytes_to_send[3] = 0;
            bytes_to_send[4] = (byte)(Thorlabs.APT.Address.MOTHERBOARD | 0x80);
            bytes_to_send[5] = (byte)(Thorlabs.APT.Address.HOST);
            bytes_to_send[6] = identifier;

            Write(bytes_to_send, 0, 7);

            if (!await data_arrived.WaitAsync(TimeSpan.FromSeconds(2)))
            {
                return null;
            }
            DataReceived -= f;

            if (data_available[1] == 0)
            {
                return null;
            }
            bool OWNED = data_available[2] > 0;
            byte ATTRIBUTES = data_available[3];
            ushort SIZE = BitConverter.ToUInt16(data_available, 4);

            EFSFile? to_remove = GetFile(identifier);
            bool update = to_remove is not null;

            EFSFile file;
            if (update)
            {
                _files.Remove(to_remove);
                file = new(this, identifier, OWNED,
                    ATTRIBUTES, SIZE);
                _files.Add(file);
            }
            else
            {
                OnFileAdded(identifier, OWNED, ATTRIBUTES, SIZE);
                file = GetFile(identifier);
            }

            FilesChanged?.Invoke(this, new());

            return file;
        }

        public EFSFile? GetFile(uint extension, uint id) => GetFile((byte)((extension << 5) | (id & 0x1F)));
        public EFSFile? GetFile(byte identifier)
        {
            EFSFile? rt = null;

            foreach (var item in _files)
            {
                if (identifier == item.Identifier)
                {
                    rt = item;
                    break;
                }
            }

            return rt;
        }

        public Task<EFSFile?> CreateFile(uint extension, uint id, EFSAttributes attributes, uint byte_size) =>
            CreateFile((byte)((extension << 5) | (id & 0x1F)), attributes, byte_size);
        public async Task<EFSFile?> CreateFile(byte ident, EFSAttributes attributes, uint byte_size)
        {
            uint PAGES = (byte_size + PageSize - 1) / PageSize;

            byte[] bytes_to_send = new byte[6 + 4];
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_EFS_SET_FILEINFO), 0, bytes_to_send, 0, 2);
            bytes_to_send[2] = 4;
            bytes_to_send[3] = 0;
            bytes_to_send[4] = (byte)((byte)Thorlabs.APT.Address.MOTHERBOARD | 0x80);
            bytes_to_send[5] = (byte)Thorlabs.APT.Address.HOST;
            bytes_to_send[6] = ident;
            bytes_to_send[7] = (byte)attributes;
            Array.Copy(BitConverter.GetBytes(PAGES), 0, bytes_to_send, 8, 2);

            Write(bytes_to_send, 0, 10);

            EFSFile? file = await LookupFileAsync(ident);

            if (file is not null)
            {
                --FilesRemaining;
                PagesRemaining -= PAGES;
            }

            return file;
        }

        public Task<bool> DeleteFileAsync(uint extension, uint id) => DeleteFileAsync((byte)((extension << 5) | (id & 0x1F)));
        public async Task<bool> DeleteFileAsync(byte ident)
        {
            EFSFile? file = GetFile(ident);
            if (file is null)
            {
                return false;
            }

            byte[] bytes_to_send = new byte[6 + 4];
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_EFS_SET_FILEINFO), 0, bytes_to_send, 0, 2);
            bytes_to_send[2] = 4;
            bytes_to_send[3] = 0;
            bytes_to_send[4] = (byte)((byte)Thorlabs.APT.Address.MOTHERBOARD | 0x80);
            bytes_to_send[5] = (byte)Thorlabs.APT.Address.HOST;
            bytes_to_send[6] = ident;
            bytes_to_send[7] = 0;
            bytes_to_send[8] = 0;
            bytes_to_send[9] = 0;

            Write(bytes_to_send, 0, 10);

            bool MISSING = await LookupFileAsync(ident) is null;
            if (MISSING)
            {
                // Remove the file.
                _files.Remove(file);

                FilesChanged?.Invoke(this, new());
                ++FilesRemaining;
                PagesRemaining += file.PageLength;
                HeaderChanged?.Invoke(this, new());
            }

            return MISSING;
        }

        public delegate void USBWrite(byte[] bytes_to_send, int offset, int length);
        internal USBWrite Write;

        public event EventHandler HeaderChanged;
        public event EventHandler FilesChanged;    
        private event EventHandler<byte[]> DataReceived;

        public EFSDriver(USBWrite writer)
        {
            Write = writer;
        }

        public void Reset()
        {
            _files.Clear();
            FilesChanged?.Invoke(this, new());
        }

        public void UpdateHwInfo(
            uint programming_payload_limit
        ) {
            ProgrammingPayloadLimit = programming_payload_limit;
        }

        public void UpdateHeaderInfo(
            string identifier,
            uint file_version,
            uint page_size,
            uint total_pages,
            uint max_files,
            uint files_remaining,
            uint pages_remaining
        ) {
            Identifier = identifier;
            FileVersion = file_version;
            PageSize = page_size;
            TotalPages = total_pages;
            MaximumFiles = max_files;
            FilesRemaining = files_remaining;
            PagesRemaining = pages_remaining;

            HeaderChanged?.Invoke(this, new EventArgs());
        }

        private void OnFileAdded(
            byte identifier,
            bool file_owned,
            byte attributes,
            uint pages
        ) {
            EFSFile file = new(this, identifier, file_owned, attributes, pages);
            _files.Add(file);
            FilesChanged?.Invoke(this, new EventArgs());
            HeaderChanged?.Invoke(this, new());
        }
    
        public void OnDataRecieved(ushort command, byte[] data)
        {
            switch(command)
            {
                case Thorlabs.APT.MGMSG_MCM_EFS_GET_FILEDATA:
                    foreach (var obj in _files)
                    {
                        obj.RouteEvent(this, data);
                    }
                break;
                case Thorlabs.APT.MGMSG_MCM_EFS_GET_FILEINFO:
                    DataReceived?.Invoke(this, data);
                break;
            }
        }
    }

    public partial class MainWindow
    {
        private EFSDriver _efs_driver;
        private bool _efs_updating_ui = true;

        private void efs_refresh_ui() => Dispatcher.Invoke(() =>
        {

            // Refresh Header
            e_lbl_efs_identifier.Content = _efs_driver.Identifier;
            e_lbl_efs_version.Content = _efs_driver.FileVersion;
            e_lbl_efs_page_size.Content = _efs_driver.PageSize;
            e_lbl_efs_packet_limit.Content = _efs_driver.ProgrammingPayloadLimit;
            e_lbl_efs_pages_remaining.Content = _efs_driver.PagesRemaining;
            e_lbl_efs_total_pages.Content = _efs_driver.TotalPages;
            e_lbl_efs_files_remaining.Content = _efs_driver.FilesRemaining;
            e_lbl_efs_total_files.Content = _efs_driver.MaximumFiles;
            e_pbr_efs_pages_remaining.Value = _efs_driver.TotalPages == 0 ? 0 : ((float)(_efs_driver.TotalPages - _efs_driver.PagesRemaining) / _efs_driver.TotalPages);
            e_pbr_efs_files_remaining.Value = _efs_driver.MaximumFiles == 0 ? 0 : ((float)(_efs_driver.MaximumFiles - _efs_driver.FilesRemaining) / _efs_driver.MaximumFiles);



            // Refresh Files
            var header = e_stk_efs_files.Children[0];
            e_stk_efs_files.Children.Clear();
            e_stk_efs_files.Children.Add(header);

            var files = _efs_driver.Files;
            foreach (var file in files)
            {
                EFSFileDisplay display = new(file);
                e_stk_efs_files.Children.Add(display);
            }

        });

        private async void Button_EFSUpload_Clicked(object? sender, RoutedEventArgs args)
        {
            if (_efs_driver.FilesRemaining == 0)
            {
                MessageBox.Show("EFS is storing all of its files", "Out of Storage");
                return;
            }

            uint extension;

            ComboBoxItem? item = e_cbx_efs_selected_extension.SelectedItem as ComboBoxItem;
            if (item is null)
            {
                return;
            }
            switch(item.Content)
            {
                case "LUT":
                    extension = 3;
                    break;
                case "User":
                    extension = 7;
                    break;
                default:
                    MessageBox.Show("Unknown selected value: " + item.Content);
                    return;
            }
            uint id;
            if (!uint.TryParse(e_tbx_efs_selected_id.Text, out id))
            {
                MessageBox.Show("Could not parse \"" + e_tbx_efs_selected_id.Text + "\" into an integer.");
                return;
            }
            EFSAttributes attr = new(
                e_chk_efs_selected_er.IsChecked == true,
                e_chk_efs_selected_ew.IsChecked == true,
                e_chk_efs_selected_ed.IsChecked == true,
                e_chk_efs_selected_fr.IsChecked == true,
                e_chk_efs_selected_fw.IsChecked == true,
                e_chk_efs_selected_fd.IsChecked == true
            );
            // Verify correct inputs
            uint ident = (byte)(id | (extension << 5));



            OpenFileDialog ofd = new();
            ofd.Title = "Upload file to \"" + ident.ToString() + "\"...";
            ofd.DefaultExt = ".bin";
            ofd.Filter = "Binary Files|*.bin";
            bool? result = ofd.ShowDialog();
            if (result is null || !(bool)result)
            {
                return;
            }

            // Read in Data
            string path = ofd.FileName;
            byte[] data;
            try
            {
                data = System.IO.File.ReadAllBytes(path);
            }
            catch (Exception e)
            {
                MessageBox.Show("File error: " + e.Message, "File Error");
                return;
            }


            // Calculate if possible
            long pages = (data.Length + _efs_driver.PageSize - 1) / _efs_driver.PageSize;
            if (pages > _efs_driver.PagesRemaining)
            {
                MessageBox.Show(string.Format("Pages to Allocate: {0}\nPages Remaining: {1}", pages, _efs_driver.PagesRemaining), "Out of Space");
                return;
            }

            // Try to create file
            EFSFile? file = await _efs_driver.CreateFile(extension, id, attr, (uint)data.Length);

            // See if file was made
            if (file is null)
            {
                MessageBox.Show("Could not create the file","File Creation Error");
                return;
            }

            // Write to file
            file.Write(0, data);
            // Verify file
            byte[] read = await file.ReadAsync(0, data.Length);
            for (int i = 0; i < data.Length; ++i)
            {
                if (read[i] != data[i])
                {
                    MessageBox.Show("Byte Mismatch in File\nAddress: " + i + "\nExpected: " + data[i] + "\nActual: " + read[i], "Verify Failed");
                    return;
                }
            }

            MessageBox.Show("Files has been written successfully", "Success");
        }

        private async void Button_EFSRefreshAll_Clicked(object? sender, RoutedEventArgs args)
        {
            _efs_updating_ui = false;
            await _efs_driver.LookupAllFilesAsync();
            _efs_updating_ui = true;
            efs_refresh_ui();
        }

    }
}