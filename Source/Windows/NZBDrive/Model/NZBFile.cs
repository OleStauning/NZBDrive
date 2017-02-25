using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public class NZBFile : INotifyPropertyChanged
    {
        private string _fileName;
        public string FileName 
        {
            get { return _fileName; }
            set { _fileName = value; OnPropertyChanged(); }
        }
        private int _parts;
        public int Parts 
        {
            get { return _parts; }
            set { _parts = value; OnPropertyChanged(); }
        }

        private int _partsLoaded = 0;
        public int PartsLoaded 
        {
            get { return _partsLoaded; }
            set 
            { 
                _partsLoaded = value; 
                OnPropertyChanged();
                if (MountStatus == Model.MountStatus.Mounted) SetMountedProgress();
            }
        }

        private ulong _size;
        public ulong Size
        {
            get { return _size; }
            set 
            { 
                _size = value;
                ulong size = _size;
                if ((size /= 1024) < 1024 * 10) { FileSize = size; FileSizeUnit = DataSizeUnit.KiB; }
                else if ((size /= 1024) < 1024 * 10) { FileSize = size; FileSizeUnit = DataSizeUnit.MiB; }
                else { FileSize = (size/=1024); FileSizeUnit = DataSizeUnit.GiB; }
                OnPropertyChanged(); 
            }
        }
        private ulong _fileSize;
        public ulong FileSize
        {
            get { return _fileSize; }
            set { _fileSize = value; OnPropertyChanged(); }
        }
        private DataSizeUnit _fileSizeUnit;
        public DataSizeUnit FileSizeUnit
        {
            get { return _fileSizeUnit; }
            private set { _fileSizeUnit = value; OnPropertyChanged(); }
        }
        private int _progress;
        public int Progress 
        {
            get { return _progress; }
            set { _progress = value; OnPropertyChanged(); }
        }

        private MountStatus _mountStatus = MountStatus.Mounting;
        public MountStatus MountStatus
        {
            get { return _mountStatus; }
            set { _mountStatus = value; OnPropertyChanged(); }
        }

        public NZBFilePartCollection Content { get; set; }

        private void SetMountedProgress()
        {
            Progress = (100 * PartsLoaded) / Parts;        
        }

        private int _partsMissing=0;
        public int PartsMissing
        {
            get { return _partsMissing; }
            set { _partsMissing = value; OnPropertyChanged(); OnPropertyChanged("FileStatus"); }
        }

        public NZBFileStatus FileStatus
        {
            get 
            {
                return _partsMissing == 0 ? NZBFileStatus.OK : NZBFileStatus.Error;
            }
        }

        internal void AddPart(NZBFilePart file)
        {
            Content.Add(file);
            Size += file.Size;
            Parts += file.Segments;
        }

        internal void SetMountingStatus(int parts, int total)
        {
            Progress = (100*parts)/total;
            if (parts == total) 
            {
                SetMountedProgress();
                MountStatus = Model.MountStatus.Mounted;
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null) handler(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
