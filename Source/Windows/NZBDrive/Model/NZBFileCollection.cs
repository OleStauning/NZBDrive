using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public class NZBFileList : ObservableCollection<NZBFile> 
    {
        private NZBDriveDLL.NZBDrive _nzbDrive;
        private Dictionary<int, NZBFile> _nzbFileMap = new Dictionary<int, NZBFile>();
        private Dictionary<int, NZBFilePart> _filePartMap = new Dictionary<int, NZBFilePart>();

        public NZBFileList(NZBDriveDLL.NZBDrive nzbDrive)
            : base()
        {
            _nzbDrive = nzbDrive;
        }

        public NZBFileList(NZBDriveDLL.NZBDrive nzbDrive, IEnumerable<NZBFile> value)
            : base(value)
        {
            _nzbDrive = nzbDrive;
        }

        internal void NZBFileOpenEvent(int nzbID, string path)
        {
            var file = new NZBFile()
            {
                FileName = path,
                Parts = 0,
                Size = 0,
                Progress = 0,
                Content = new NZBFilePartCollection()
            };
            _nzbFileMap[nzbID] = file;
            this.Add(file);
        }

        internal void NZBFileCloseEvent(int nzbID)
        {
            this.Remove(_nzbFileMap[nzbID]);
            _nzbFileMap.Remove(nzbID);
        }

        internal void FileAddedEvent(int nzbID, int fileID, int segments, ulong size)
        {
            NZBFile nzbFile;

            if (_nzbFileMap.TryGetValue(nzbID, out nzbFile))
            {
                var nzbPart = new NZBFilePart(nzbID, segments, size);

                _filePartMap[fileID] = nzbPart;

                nzbFile.AddPart(nzbPart);            
            }

        }

        internal void FileRemovedEvent(int fileID)
        {
//            nzbFile.RemovePart(_filePartMap[fileID]);

        }

        internal void FileInfoEvent(int fileID, string name, ulong size)
        {
            NZBFilePart nzbPart;

            if (_filePartMap.TryGetValue(fileID, out nzbPart))
            {
                nzbPart.FileInfoEvent(name, size);

                NZBFile nzbFile;

                if (_nzbFileMap.TryGetValue(nzbPart.NZBID, out nzbFile))
                {
                    nzbFile.Size += size - nzbPart.Size;
                }
            }

        }

        internal void FileSegmentStateChangedEvent(int fileID, int segment, NZBDriveDLL.SegmentState state)
        {
            NZBFilePart nzbPart;

            if (_filePartMap.TryGetValue(fileID, out nzbPart))
            {
                nzbPart.SetSegmentStatus(segment, state);

                NZBFile nzbFile;
                if (_nzbFileMap.TryGetValue(nzbPart.NZBID, out nzbFile))
                {
                    if (state == NZBDriveDLL.SegmentState.HasData)
                    {
                        nzbFile.PartsLoaded++;
                    }
                    else if (state == NZBDriveDLL.SegmentState.DownloadFailed || state==NZBDriveDLL.SegmentState.MissingSegment)
                    {
                        nzbFile.PartsMissing++;
                    }
                }
            }
        }

        internal void MountStatus(int nzbID, int parts, int total)
        {
            NZBFile nzbFile;

            if (_nzbFileMap.TryGetValue(nzbID, out nzbFile))
            {
                nzbFile.SetMountingStatus(parts, total);            
            }

        }
        /*
        internal void Unmount(int idx)
        {
            _nzbDrive.Unmount(this[idx].FileName);
            RemoveAt(idx);
        }
        */
        internal void Unmount(NZBFile file)
        {
            _nzbDrive.Unmount(file.FileName);
            Remove(file);
        }
    }
}
