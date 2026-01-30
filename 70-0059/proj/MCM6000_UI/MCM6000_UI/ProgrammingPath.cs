using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;

namespace MCM6000_UI
{
    /// <summary>
    /// Simple interface around a path
    /// that checks what the path can program (file attributes).
    /// Attributes are lazy and are only calculated when requested.
    /// </summary>
    internal sealed class ProgrammingPath
    {
        public string Path
        {
            get => _path;
            set
            {
                if (value == _path)
                { return; }
                _path = value;
                _fileAttributesComputed = false;
            }
        }
        
        /// <summary>
        /// Used for .mcmpkg files.
        /// </summary>
        public string EncryptionKey
        { get; set; } = string.Empty;

        public bool ProvidesFirmware
        {
            get
            {
                ComputeAttributes();
                return _hasFirmware;
            }
        }

        public bool ProvidesCPLD
        {
            get
            {
                ComputeAttributes();
                return _hasCPLD;
            }
        }

        public bool ProvidesLUT
        {
            get
            {
                ComputeAttributes();
                return _hasLUT;
            }
        }

        public IEnumerable<byte> LUTKeys
        {
            get
            {
                ComputeAttributes();
                return _lutKeys;
            }
        }


        public Stream OpenFirmware()
        {
            ComputeAttributes();
            if (!ProvidesFirmware)
            {
                throw new InvalidOperationException("file does not provide firmware");
            }

            if (_pkgData is null)
            {
                // .hex
                return File.OpenRead(Path);
            }
            else
            {
                // .mcmpkg
                return DependentStream.Open(OpenPackage(), x => x.OpenRead(_pkgData.FirmwareKey));
            }
        }
        public Stream OpenCPLD()
        {
            ComputeAttributes();
            if (!ProvidesCPLD)
            {
                throw new InvalidOperationException("files does not provide CPLD");
            }

            if (_pkgData is null)
            {
                // .jed
                return File.OpenRead(Path);
            }
            else
            {
                // .mcmpkg
                return DependentStream.Open(OpenPackage(), x => x.OpenRead(_pkgData.CpldKey));
            }
        }
        public Stream OpenLUT(byte fileId)
        {
            ComputeAttributes();
            if (!ProvidesLUT)
            {
                throw new InvalidOperationException("file does not provide lut");
            }
            if (!LUTKeys.Contains(fileId))
            {
                throw new ArgumentException($"LUT file does not contain key for ID 0x{fileId:X02}", nameof(fileId));
            }

            if (_pkgData is null)
            {
                // .zip
                ZipArchive archive = null;
                var stream = File.OpenRead(Path);
                try
                {
                    archive = new(stream, ZipArchiveMode.Read);
                    return DependentStream.Open(archive, x =>
                    {
                        var entry = x.GetEntry(fileId switch
                        {
                            96 => "device-lut.bin",
                            97 => "config-lut.bin",
                            98 => "lut-version.bin",
                            _ => throw new NotImplementedException("program invalid"),
                        });
                        return entry.Open();
                    }, false); // Upper block handles disposing of the archive.
                }
                catch
                {
                    archive?.Dispose();
                    stream.Dispose();
                    throw;
                }
            }
            else
            {
                // .mcmpkg
                return DependentStream.Open(OpenPackage(), x => x.OpenRead(_pkgData.LutKeys[fileId]));
            }
        }


        private void ComputeAttributes()
        {
            if (_fileAttributesComputed) { return; }
            string ext = System.IO.Path.GetExtension(Path);
            bool isExtSupported = ext switch
            {
                ".hex" => true,
                ".jed" => true,
                ".mcmpkg" => true,
                ".zip" => true,
                _ => false,
            };
            if (isExtSupported && File.Exists(Path))
            {
                if (ext == ".mcmpkg")
                {
                    _pkgData = new();

                    try
                    {
                        using var reader = Thorlabs.MCMPackage.OpenReader(Path);

                        var firmwareProgrammer = reader.Device.Programmables.SingleOrDefault(x => x is Thorlabs.ThorDeviceSchemaGraph.Programmables.S70FirmwareTarget);
                        if (firmwareProgrammer is not null)
                        {
                            _pkgData.FirmwareKey = reader.GetLinkedPath(firmwareProgrammer.Name);
                        }

                        var cpldProgrammer = reader.Device.Programmables.SingleOrDefault(x => x is Thorlabs.ThorDeviceSchemaGraph.Programmables.McmCpldTarget);
                        if (cpldProgrammer is not null)
                        {
                            _pkgData.CpldKey = reader.GetLinkedPath(cpldProgrammer.Name);
                        }

                        var lutProgrammer = reader.Device.Programmables.OfType<Thorlabs.ThorDeviceSchemaGraph.Programmables.McmLutTarget>().SingleOrDefault();
                        foreach (var key in lutProgrammer?.FileNames ?? [])
                        {
                            var efsProgrammer = reader.Device.FindProgrammable(key) as Thorlabs.ThorDeviceSchemaGraph.Programmables.EfsFileTarget;
                            if (efsProgrammer is not null)
                            {
                                _pkgData.LutKeys.Add(efsProgrammer.Id, reader.GetLinkedPath(key));
                            }
                        }
                    }
                    catch (Exception)
                    {
#if DEBUG
                        Debugger.Break();
#endif
                    }

                    _hasFirmware = _pkgData.FirmwareKey.Length > 0;
                    _hasCPLD = _pkgData.CpldKey.Length > 0;
                    _hasLUT = _pkgData.LutKeys.Count > 0;
                    _lutKeys = _pkgData.LutKeys.Keys;
                }
                else
                {
                    _pkgData = null;

                    _hasFirmware = ext == ".hex";
                    _hasCPLD = ext == ".jed";

                    if (ext == ".zip")
                    {
                        try
                        {
                            using var stream = File.OpenRead(Path);
                            using var archive = new ZipArchive(stream, ZipArchiveMode.Read);

                            string[] NAMES = [
                                "config-lut.bin",
                                "device-lut.bin",
                                "lut-version.bin",
                            ];
                            _hasLUT = archive.Entries.Select(x => x.Name).Intersect(NAMES).Count() == NAMES.Count();
                            _lutKeys = [96, 97, 98];
                        }
                        catch (Exception)
                        {
#if DEBUG
                            Debugger.Break();
#endif
                        }
                    }
                    else
                    {
                        _hasLUT = false;
                        _lutKeys = [];
                    }
                }
            }
            _fileAttributesComputed = true;
        }

        private class PackageConfig
        {
            public string FirmwareKey = string.Empty;
            public string CpldKey = string.Empty;
            public Dictionary<byte, string> LutKeys = [];
        };


        private bool _fileAttributesComputed = false;
        private bool _hasFirmware = false;
        private bool _hasCPLD = false;
        private bool _hasLUT = false;
        private IEnumerable<byte> _lutKeys = [];
        private PackageConfig _pkgData = null;

        private string _path = string.Empty;

        private Thorlabs.MCMPack.IPackageReader OpenPackage()
        {
            var rt = Thorlabs.MCMPackage.OpenReader(Path);
            rt.EncryptionKey = EncryptionKey;
            return rt;
        }
    }
}
