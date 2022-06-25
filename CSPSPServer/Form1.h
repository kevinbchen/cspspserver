#pragma once
#include "GameServer.h"
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <chrono>
#include <thread>

namespace CSPSPServer {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Diagnostics;
	using namespace System::ComponentModel;
	using namespace System::Runtime::InteropServices;

	static GameServer* server;
	static std::chrono::time_point<std::chrono::steady_clock> currenttime;
	static char input[1024];
	static bool mIsInputWaiting;

	public delegate void OnListUpdateDelegate();
	static GCHandle playerListUpdateHandle;
	static GCHandle banListUpdateHandle;

	/// <summary>
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{

	public:
		Form1(void)
		{
			InitializeComponent();

			server = new GameServer();
			currenttime = std::chrono::steady_clock::now();
			mIsInputWaiting = false;

			// Redirect stdout to a stringstream
			std::cout.rdbuf(server->mOutStream.rdbuf());

			// Set player list and ban list update callbacks.
			// Note that we keep GCHandles to ensure the function pointers stay valid.
			OnListUpdateDelegate^ playerListUpdateDelegate = gcnew OnListUpdateDelegate(this, &Form1::onPlayerListUpdate);
			OnListUpdateDelegate^ banListUpdateDelegate = gcnew OnListUpdateDelegate(this, &Form1::onBanListUpdate);
			playerListUpdateHandle = GCHandle::Alloc(playerListUpdateDelegate);
			banListUpdateHandle = GCHandle::Alloc(banListUpdateDelegate);
			server->mOnPlayerListUpdate = static_cast<void (*)(void)>(Marshal::GetFunctionPointerForDelegate(playerListUpdateDelegate).ToPointer());
			server->mOnBanListUpdate = static_cast<void (*)(void)>(Marshal::GetFunctionPointerForDelegate(banListUpdateDelegate).ToPointer());

			server->Init();

			// Main update loop
			Application::Idle += gcnew System::EventHandler(this, &Form1::UpdateOutput);
		}

		void UpdateOutput(Object^ /*myObject*/, EventArgs^ /*myEventArgs*/) {
			MSG msg = {};
			while (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE) == 0) {
				auto now = std::chrono::steady_clock::now();
				std::chrono::duration<double> diff = now - currenttime;
				float dt = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(now - currenttime).count();
				currenttime = now;

				if (!server->mHasError) {
					server->Update(dt);
				}

				std::string output = server->mOutStream.str();
				if (output.length() > 0) {
					server->mOutStream.str("");

					String^ systemString = gcnew String(output.c_str());
					outputBox->AppendText(systemString);
					outputBox->ScrollToCaret();
				}
				std::this_thread::sleep_until(now + std::chrono::milliseconds(10));
			}
		}

		void onPlayerListUpdate() {
			playerListBox->Items->Clear();
			for (auto* player : server->mPeople) {
				playerListBox->Items->Add(gcnew System::String(player->mName));
			}
		}

		void onBanListUpdate() {
			banListBox->Items->Clear();
			for (auto* name : server->mBannedPeople) {
				banListBox->Items->Add(gcnew System::String(name));
			}
		}


	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			delete server;

			playerListUpdateHandle.Free();
			banListUpdateHandle.Free();

			if (components)
			{
				delete components;
			}
		}


	private: System::Windows::Forms::RichTextBox^  outputBox;
	private: System::Windows::Forms::TextBox^  inputBox;
	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::NotifyIcon^  notifyIcon1;
	private: System::Windows::Forms::ContextMenuStrip^  contextMenuStrip1;
	private: System::Windows::Forms::ToolStripMenuItem^  restoreToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  exitToolStripMenuItem;
	private: System::Windows::Forms::ListBox^  playerListBox;
	private: System::Windows::Forms::Button^  kickButton;
	private: System::Windows::Forms::Button^  banButton;
	private: System::Windows::Forms::ListBox^  banListBox;
	private: System::Windows::Forms::GroupBox^  groupBox1;
	private: System::Windows::Forms::GroupBox^  groupBox2;
	private: System::Windows::Forms::Button^  unbanButton;
	private: System::ComponentModel::IContainer^  components;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			System::ComponentModel::ComponentResourceManager^ resources = (gcnew System::ComponentModel::ComponentResourceManager(Form1::typeid));
			this->outputBox = (gcnew System::Windows::Forms::RichTextBox());
			this->inputBox = (gcnew System::Windows::Forms::TextBox());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->notifyIcon1 = (gcnew System::Windows::Forms::NotifyIcon(this->components));
			this->contextMenuStrip1 = (gcnew System::Windows::Forms::ContextMenuStrip(this->components));
			this->restoreToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->exitToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->playerListBox = (gcnew System::Windows::Forms::ListBox());
			this->kickButton = (gcnew System::Windows::Forms::Button());
			this->banButton = (gcnew System::Windows::Forms::Button());
			this->banListBox = (gcnew System::Windows::Forms::ListBox());
			this->groupBox1 = (gcnew System::Windows::Forms::GroupBox());
			this->groupBox2 = (gcnew System::Windows::Forms::GroupBox());
			this->unbanButton = (gcnew System::Windows::Forms::Button());
			this->contextMenuStrip1->SuspendLayout();
			this->groupBox1->SuspendLayout();
			this->groupBox2->SuspendLayout();
			this->SuspendLayout();
			// 
			// outputBox
			// 
			this->outputBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->outputBox->DetectUrls = false;
			this->outputBox->Location = System::Drawing::Point(12, 12);
			this->outputBox->Name = L"outputBox";
			this->outputBox->ReadOnly = true;
			this->outputBox->Size = System::Drawing::Size(395, 266);
			this->outputBox->TabIndex = 2;
			this->outputBox->Text = L"";
			// 
			// inputBox
			// 
			this->inputBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->inputBox->Location = System::Drawing::Point(28, 287);
			this->inputBox->MaxLength = 125;
			this->inputBox->Name = L"inputBox";
			this->inputBox->Size = System::Drawing::Size(485, 20);
			this->inputBox->TabIndex = 3;
			this->inputBox->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &Form1::inputBox_KeyDown);
			// 
			// label1
			// 
			this->label1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(9, 290);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(13, 13);
			this->label1->TabIndex = 4;
			this->label1->Text = L">";
			// 
			// notifyIcon1
			// 
			this->notifyIcon1->ContextMenuStrip = this->contextMenuStrip1;
			this->notifyIcon1->Icon = (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"notifyIcon1.Icon")));
			this->notifyIcon1->Text = L"CSPSPServer";
			this->notifyIcon1->MouseDoubleClick += gcnew System::Windows::Forms::MouseEventHandler(this, &Form1::notifyIcon1_MouseDoubleClick);
			// 
			// contextMenuStrip1
			// 
			this->contextMenuStrip1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(2) {
				this->restoreToolStripMenuItem,
					this->exitToolStripMenuItem
			});
			this->contextMenuStrip1->Name = L"contextMenuStrip1";
			this->contextMenuStrip1->Size = System::Drawing::Size(114, 48);
			// 
			// restoreToolStripMenuItem
			// 
			this->restoreToolStripMenuItem->Name = L"restoreToolStripMenuItem";
			this->restoreToolStripMenuItem->Size = System::Drawing::Size(113, 22);
			this->restoreToolStripMenuItem->Text = L"Restore";
			this->restoreToolStripMenuItem->Click += gcnew System::EventHandler(this, &Form1::restoreToolStripMenuItem_Click);
			// 
			// exitToolStripMenuItem
			// 
			this->exitToolStripMenuItem->Name = L"exitToolStripMenuItem";
			this->exitToolStripMenuItem->Size = System::Drawing::Size(113, 22);
			this->exitToolStripMenuItem->Text = L"Exit";
			this->exitToolStripMenuItem->Click += gcnew System::EventHandler(this, &Form1::exitToolStripMenuItem_Click);
			// 
			// playerListBox
			// 
			this->playerListBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->playerListBox->FormattingEnabled = true;
			this->playerListBox->HorizontalScrollbar = true;
			this->playerListBox->Location = System::Drawing::Point(6, 19);
			this->playerListBox->Name = L"playerListBox";
			this->playerListBox->Size = System::Drawing::Size(88, 108);
			this->playerListBox->TabIndex = 5;
			// 
			// kickButton
			// 
			this->kickButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->kickButton->Location = System::Drawing::Point(6, 132);
			this->kickButton->Name = L"kickButton";
			this->kickButton->Size = System::Drawing::Size(45, 19);
			this->kickButton->TabIndex = 6;
			this->kickButton->Text = L"kick";
			this->kickButton->UseVisualStyleBackColor = true;
			this->kickButton->Click += gcnew System::EventHandler(this, &Form1::kickButton_Click);
			// 
			// banButton
			// 
			this->banButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->banButton->Location = System::Drawing::Point(51, 132);
			this->banButton->Name = L"banButton";
			this->banButton->Size = System::Drawing::Size(43, 19);
			this->banButton->TabIndex = 7;
			this->banButton->Text = L"ban";
			this->banButton->UseVisualStyleBackColor = true;
			this->banButton->Click += gcnew System::EventHandler(this, &Form1::banButton_Click);
			// 
			// banListBox
			// 
			this->banListBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Right));
			this->banListBox->FormattingEnabled = true;
			this->banListBox->Location = System::Drawing::Point(7, 22);
			this->banListBox->Name = L"banListBox";
			this->banListBox->Size = System::Drawing::Size(87, 69);
			this->banListBox->TabIndex = 10;
			// 
			// groupBox1
			// 
			this->groupBox1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->groupBox1->Controls->Add(this->playerListBox);
			this->groupBox1->Controls->Add(this->kickButton);
			this->groupBox1->Controls->Add(this->banButton);
			this->groupBox1->Location = System::Drawing::Point(413, 5);
			this->groupBox1->Name = L"groupBox1";
			this->groupBox1->Size = System::Drawing::Size(100, 157);
			this->groupBox1->TabIndex = 11;
			this->groupBox1->TabStop = false;
			this->groupBox1->Text = L"Current Players";
			// 
			// groupBox2
			// 
			this->groupBox2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Right));
			this->groupBox2->Controls->Add(this->unbanButton);
			this->groupBox2->Controls->Add(this->banListBox);
			this->groupBox2->Location = System::Drawing::Point(413, 162);
			this->groupBox2->Name = L"groupBox2";
			this->groupBox2->Size = System::Drawing::Size(99, 119);
			this->groupBox2->TabIndex = 12;
			this->groupBox2->TabStop = false;
			this->groupBox2->Text = L"Banned Players";
			// 
			// unbanButton
			// 
			this->unbanButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->unbanButton->Location = System::Drawing::Point(7, 94);
			this->unbanButton->Name = L"unbanButton";
			this->unbanButton->Size = System::Drawing::Size(88, 19);
			this->unbanButton->TabIndex = 11;
			this->unbanButton->Text = L"unban";
			this->unbanButton->UseVisualStyleBackColor = true;
			this->unbanButton->Click += gcnew System::EventHandler(this, &Form1::unbanButton_Click);
			// 
			// Form1
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(525, 316);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->inputBox);
			this->Controls->Add(this->outputBox);
			this->Controls->Add(this->groupBox1);
			this->Controls->Add(this->groupBox2);
			this->Icon = (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"$this.Icon")));
			this->Name = L"Form1";
			this->Text = L"CSPSPServer";
			this->Resize += gcnew System::EventHandler(this, &Form1::Form1_Resize);
			this->contextMenuStrip1->ResumeLayout(false);
			this->groupBox1->ResumeLayout(false);
			this->groupBox2->ResumeLayout(false);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void inputBox_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
				if (e->KeyCode == Keys::Enter) {
					//if (!mIsInputWaiting) {
						strcpy(input,"");
						for (int i=0; i<inputBox->Text->Length; i++) {
							input[i] = inputBox->Text[i];
						}
						input[inputBox->Text->Length] = '\0';
						//mIsInputWaiting = true;
						server->HandleInput(input);
						inputBox->Clear();
					//}
				}
				//if (e->KeyCode == Keys::) {
			}

private: System::Void inputBox_KeyPress(System::Object^  sender, System::Windows::Forms::KeyPressEventArgs^  e) {
		 }

private: System::Void Form1_Resize(System::Object^  sender, System::EventArgs^  e) {
			if ( this->WindowState == FormWindowState::Minimized) {
				this->Hide();
				notifyIcon1->Visible = true;
			}
		 }
private: System::Void notifyIcon1_MouseDoubleClick(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
			if ( this->WindowState == FormWindowState::Minimized) {
				this->Show();
				this->WindowState = FormWindowState::Normal;
			}
			notifyIcon1->Visible = false;
			this->Activate();
		 }
private: System::Void restoreToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
			if ( this->WindowState == FormWindowState::Minimized) {
				this->Show();
				this->WindowState = FormWindowState::Normal;
			}
			notifyIcon1->Visible = false;
			this->Activate();
		 }
private: System::Void exitToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
			 this->Close();
		 }
private: System::Void kickButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 for (int i=0; i<playerListBox->Items->Count; i++) {
				 if (playerListBox->GetSelected(i)) {
					 char name[32];
					 for (int j=0; j<playerListBox->Items[i]->ToString()->Length; j++) {
						name[j] = playerListBox->Items[i]->ToString()[j];
					 }
					 name[playerListBox->Items[i]->ToString()->Length] = '\0';
					 server->Kick(name);
					 break;
				 }
			 }
		 }
private: System::Void banButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 for (int i=0; i<playerListBox->Items->Count; i++) {
				 if (playerListBox->GetSelected(i)) {
					 char name[32];
					 for (int j=0; j<playerListBox->Items[i]->ToString()->Length; j++) {
						name[j] = playerListBox->Items[i]->ToString()[j];
					 }
					 name[playerListBox->Items[i]->ToString()->Length] = '\0';
					 server->Ban(name);
					 break;
				 }
			 }
		 }
private: System::Void unbanButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 for (int i=0; i<banListBox->Items->Count; i++) {
				 if (banListBox->GetSelected(i)) {
					 char name[32];
					 for (int j=0; j<banListBox->Items[i]->ToString()->Length; j++) {
						name[j] = banListBox->Items[i]->ToString()[j];
					 }
					 name[banListBox->Items[i]->ToString()->Length] = '\0';
					 server->Unban(name);
					 break;
				 }
			 }
		 }
};

}

