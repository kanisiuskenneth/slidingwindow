## Untuk menjalankan program recvfile dan sendfile, lakukan:
1. Buka terminal dan masukkan command make.
2. Eksekusi ./recvfile dengan masukan parameter <filename> <windowsize> <buffersize> <port>
  Misalnya, ./recvfile output/test.txt 30 256 8080 berarti menerima file dan menulisnya ke output/test.txt dengan receiving window size = 30, buffer size = 256, diterima di port 8080.
3. Eksekusi ./sendfile dengan masukan parameter <filename> <windowsize> <buffersize> <destination_ip> <destination_port>
  Misalnya, ./sendfile datatest/test.txt 25 256 127.0.0.1 8080 berarti mengirim file datatest/test.txt dengan window size = 25, buffer size = 256, ke alamat dengan IP address 127.0.0.1 ke port 8080.

## Cara kerja sliding window:
1. Setelah buffer milik sender sudah penuh, data akan dikirim ke receiver.
2. Sender mengirim paket-paket tersebut dengan sebuah SeqNum.
3. Setiap kali receiver sudah menerima paket, receiver akan memeriksa nilai checksum dan seqnum terlebih dahulu sebelum mengirim ACK. Seqnum yang diterima adalah seqnum yang >= LAF (Largest Acceptable Frame), di mana LAF sendiri = LFR (Last Frame Received) + RWS (Receiving Window Size).
4. Saat sender menerima ACK, sender akan memeriksa apakah ACK tersebut adalah ACK yang "diharapkan" atau tidak. Jika tidak, sender akan me-reset window. Jika sesuai, sender akan mengirim paket selanjutnya.
5. Jika sender tidak menerima ACK sampai waktu yang ditentukan, sender akan me-reset window.
Kami tidak menggunakan fungsi yang terkait dengan sliding window pada program kami. 

### Fungsi terkait sliding window:
	sendto() : fungsi yang disediakan oleh sistem operasi yang digunakan untuk mengirim data kepada sebuah socket yang telah terdefinisi, melalui sebuah socket yang sebelumnya terdefinisi, kegunaan lain dari sendto adalah ketika pemanggilan fungsi maka socket yang digunakan akan otomatis melakukan bind terhadap port tersebut
	recvfrom() : fungsi yang disediakan oleh sistem operasi yang digunakan utuk menerima data melalui socket yang telah terdefinisi, socket harus telah dibind dengan port terlebih dahulu sebelum pemanggilan fungsi

Apa yang terjadi jika advertised window yang dikirim bernilai 0? Apa cara untuk menangani hal tersebut?
Saat advertised window = x, sender tidak dapat mengirim lebih dari x byte data tanpa sebelum menerima ACK yang diharapkan. Maka, jika advertised window = 0, sender tidak dapat mengirim data ke receiver. Biasanya ini terjadi jika buffer receiver sudah penuh. Sender akan menunggu advertised window > 0 untuk mengirim data berikutnya.

Sebutkan field data yang terdapat TCP Header serta ukurannya, ilustrasikan, dan jelaskan kegunaan dari masing-masing field data tersebut!
- Source TCP port number (2 bytes): port pada pengirim data
- Destination TCP port number (2 bytes): port pada penerima data
- Sequence number (4 bytes): penanda urutan pesan pada window pada waktu tertentu
- Acknowledgment number (4 bytes): penanda keberhasilan menerima paket dengan number = seqnum paket yang diterima
- TCP data offset (4 bits): ukuran total header TCP
- Reserved data (3 bits): selalu bernilai 0, untuk mengondisikan ukuran paket berukuran kelipatan 4 byte
- Control flags (up to 9 bits): menjadi pengarah untuk mengatur flow data pada situasi-situasi spesifik
- Window size (2 bytes): menandakan ukuran window
- TCP checksum (2 bytes): sebagai fasilitas pendeteksian error pada paket
- Urgent pointer (2 bytes): dapat menandakan pesan yang memerlukan pemrosesan prioritas
- TCP optional data (0-40 bytes): mendukung acknowledgement khusus atau window scaling algorithms

## Pembagian tugas
1. Kanisius Kenneth Halim : Sliding Window
2. Helena Suzane Ringoringo : Receiver
3. Irene Edria Devina : Sender
