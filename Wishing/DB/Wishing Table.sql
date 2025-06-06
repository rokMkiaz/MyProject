CREATE TABLE [dbo].[TWishing](
	[idx] [int] IDENTITY(1,1) NOT NULL,
	[charunique] [int] NULL,
	[NecessaryTitle] [int] NULL,
	[StoneIndex] [int] NULL,
	[CSVStatIndex1] [smallint] NULL,
	[CSVStatIndex2] [smallint] NULL,
	[CSVStatIndex3] [smallint] NULL,
	[NextFlag] [smallint] NULL,
	[StoneGrade] [tinyint] NULL,
	[LockUse] [tinyint] NULL,
	[Count] [int] NULL
) ON [PRIMARY]
GO

ALTER TABLE [dbo].[TWishing] ADD  CONSTRAINT [df_TWishing_charunique]  DEFAULT ((0)) FOR [charunique]
GO

ALTER TABLE [dbo].[TWishing] ADD  CONSTRAINT [df_TWishing_stone1]  DEFAULT ((0)) FOR [NecessaryTitle]
GO

ALTER TABLE [dbo].[TWishing] ADD  CONSTRAINT [df_TWishing_stone12count]  DEFAULT ((0)) FOR [StoneIndex]
GO

ALTER TABLE [dbo].[TWishing] ADD  CONSTRAINT [DF_TWishing_CSVStatIndex1]  DEFAULT ((0)) FOR [CSVStatIndex1]
GO

ALTER TABLE [dbo].[TWishing] ADD  CONSTRAINT [DF_TWishing_CSVStatIndex2]  DEFAULT ((0)) FOR [CSVStatIndex2]
GO

ALTER TABLE [dbo].[TWishing] ADD  CONSTRAINT [DF_TWishing_CSVStatIndex3]  DEFAULT ((0)) FOR [CSVStatIndex3]
GO

ALTER TABLE [dbo].[TWishing] ADD  CONSTRAINT [DF_TWishing_NextFlag]  DEFAULT ((0)) FOR [NextFlag]
GO

ALTER TABLE [dbo].[TWishing] ADD  CONSTRAINT [DF_TWishing_StoneGrade]  DEFAULT ((0)) FOR [StoneGrade]
GO

ALTER TABLE [dbo].[TWishing] ADD  CONSTRAINT [DF_TWishing_LockUse]  DEFAULT ((0)) FOR [LockUse]
GO

ALTER TABLE [dbo].[TWishing] ADD  CONSTRAINT [DF_TWishing_Count]  DEFAULT ((0)) FOR [Count]
GO

CREATE CLUSTERED INDEX TWishing_charunique ON [TWishing]([charunique],[NecessaryTitle],[StoneIndex]) 
CREATE NONCLUSTERED INDEX TWishing_Title_StoneIndex ON [TWishing]([NecessaryTitle],[StoneIndex])
