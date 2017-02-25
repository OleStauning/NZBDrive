using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public class NZBFilePart : INotifyPropertyChanged
    {
        public NZBFilePart(int nzbID, int segments, ulong size)
        {
            NZBID = nzbID;
            FileName = "???";
            Size = size;
            Segments = segments;
            FileSegmentStatusCollection = new ObservableCollection<NZBDriveDLL.SegmentState>(Enumerable.Repeat(NZBDriveDLL.SegmentState.None, segments));
            MissingSegments = 0;
            CachedSegments = 0;
        }

        internal int NZBID { get; set; }

        private string _fileName;
        public string FileName
        {
            get { return _fileName; }
            set { _fileName = value; OnPropertyChanged(); }
        }

        private int _segments;
        public int Segments
        {
            get { return _segments; }
            set { _segments = value; OnPropertyChanged(); }
        }

        private ulong _size;
        public ulong Size 
        {
            get { return _size; }
            set { _size = value; OnPropertyChanged(); }
        }

        public ObservableCollection<NZBDriveDLL.SegmentState> FileSegmentStatusCollection { get; set; }

        private int _loadingSegments;
        public int LoadingSegments 
        { 
            get { return _loadingSegments; } 
            set { _loadingSegments = value; OnPropertyChanged(); } 
        }

        private int _missingSegments;
        public int MissingSegments 
        { 
            get { return _missingSegments; } 
            set { _missingSegments = value; OnPropertyChanged(); } 
        }

        private int _cachedSegments;
        public int CachedSegments 
        { 
            get { return _cachedSegments; } 
            set { _cachedSegments = value; OnPropertyChanged(); } 
        }

        internal void SetSegmentStatus(int segment, NZBDriveDLL.SegmentState state)
        {
            if (segment < FileSegmentStatusCollection.Count)
            {
                ChangeStatusCount(FileSegmentStatusCollection[segment], -1);
                ChangeStatusCount(state, 1);
                FileSegmentStatusCollection[segment] = state;
            }
        }

        private void ChangeStatusCount(NZBDriveDLL.SegmentState state, int delta)
        {
            switch (state)
            {
                case NZBDriveDLL.SegmentState.None: break;
                case NZBDriveDLL.SegmentState.Loading: LoadingSegments += delta; break;
                case NZBDriveDLL.SegmentState.HasData: CachedSegments += delta; break;
                case NZBDriveDLL.SegmentState.DownloadFailed: MissingSegments += delta; break;
                case NZBDriveDLL.SegmentState.MissingSegment: MissingSegments += delta; break;
            }
        }


        internal void FileInfoEvent(string name, ulong size)
        {
            FileName = name;
            Size = size;
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null) handler(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
