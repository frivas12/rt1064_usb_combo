using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Data.OleDb;

namespace MCM6000_UI
{
    internal class DB
    {

        private string DB_CONNECTION_STRING;

        private static readonly Lazy<DB> _instance = new(() => new DB());
        public static DB Instance {
            get
            {
                return _instance.Value;
            }
        }

        private DB()
        {
            string data_source = @"LUTDevices.accdb";

            OleDbConnectionStringBuilder builder = new();
            builder.Add("Data Source", data_source);
            builder.Add("Provider", "Microsoft.ACE.OLEDB.12.0");

            DB_CONNECTION_STRING = builder.ConnectionString;
        }

        public string? GetPartNumber(SystemXMLParser.Utils.Signature<ushort, ushort> device_signature)
        {
            string query = string.Format("SELECT `Part Number` from Devices WHERE `Slot Type`={0} AND `Device ID`={1};",
                device_signature.PrimaryKey.ToString(), device_signature.DiscriminatorKey.ToString());
            using (OleDbConnection conn = new(DB_CONNECTION_STRING))
            using (OleDbCommand cmd = new (query, conn))
            {
                conn.Open();
                var reader = cmd.ExecuteReader();

                string rt = "";

                if (reader.Read())
                {
                    if (reader.IsDBNull(0))
                    {
                        rt = null;
                    }
                    else
                    {
                        rt = reader.GetString(0);
                    }
                }

                reader.Close();

                return rt;
            }

            throw new Exception("Could not connect to DB");
        }
    }
}
